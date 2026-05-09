#include "scheduler.h"

// ─────────────────────────────────────────────────────────────
//  GLOBALS
// ─────────────────────────────────────────────────────────────
int msgq_id;
int sem_id;
int mem_req_qid;   // process → scheduler memory access requests
int mem_res_qid;   // scheduler → process memory responses
PCB *running_PCB = NULL;

// blocked list — processes waiting for disk to finish
#define MAX_BLOCKED 64
static PCB *blocked_list[MAX_BLOCKED];
static int  blocked_count = 0;

// ─────────────────────────────────────────────────────────────
//  BLOCKED LIST HELPERS
// ─────────────────────────────────────────────────────────────
static void enqueue_blocked(PCB *pcb) {
    if (blocked_count < MAX_BLOCKED)
        blocked_list[blocked_count++] = pcb;
}

static int blocked_list_empty() {
    return blocked_count == 0;
}

// ─────────────────────────────────────────────────────────────
//  LOG HELPER
// ─────────────────────────────────────────────────────────────
void log_event(FILE *fp, int curr_time, char *state, PCB *p, int finished) {
    int waiting = (curr_time - p->proc->arrivalTime)
                - (p->proc->runTime - p->remainingTime);
    if (finished) {
        int TA    = curr_time - p->proc->arrivalTime;
        float WTA = (p->proc->runTime > 0)
                  ? (float)TA / p->proc->runTime : 0.0f;
        fprintf(fp,
            "At time %d process %d %s arr %d total %d remain %d "
            "wait %d TA %d WTA %.2f\n",
            curr_time, p->proc->ID, state,
            p->proc->arrivalTime, p->proc->runTime,
            p->remainingTime, waiting, TA, WTA);
    } else {
        fprintf(fp,
            "At time %d process %d %s arr %d total %d remain %d wait %d\n",
            curr_time, p->proc->ID, state,
            p->proc->arrivalTime, p->proc->runTime,
            p->remainingTime, waiting);
    }
    fflush(fp);
}

// ─────────────────────────────────────────────────────────────
//  RR_DEMO
// ─────────────────────────────────────────────────────────────
void RR_DEMO(int quantum, int K) {
    FILE *out     = fopen("scheduler.log", "w");
    FILE *mem_log = fopen("memory.log",   "w");
    if (!out || !mem_log) { perror("File open failed"); exit(1); }

    // initialise MMU state
    MMU_init();

    running_PCB = NULL;

    PCBNode *rr_head;
    initPcbQueue(&rr_head);

    int quantum_remaining      = quantum;
    int generator_sent_all     = 0;
    int remainingForProcess    = 0;
    int switching              = -1;
    int requesting_ram         = -1;
    int running_process_sem_id = -1;
    int isFinished             = 0;
    int isBlocked              = 0;
    int isSamir                = 0;
    int isGranted              = 0;
    int dont_wait              = 0;

    // perf accumulators
    int   finished_processes_count = 0;
    int   total_run_time           = 0;
    float sum_WTA                  = 0.0f;
    float sum_WTA_squared          = 0.0f;
    int   sum_Wait                 = 0;

    // K-quantum R-bit reset tracking
    int quantums_elapsed = 0;

    // ── EDIT 2: staging lists so we control push order ────────
    // order into ready queue each tick:
    //   1. preempted process  (quantum expired)
    //   2. unblocked processes (disk done)
    //   3. newly arrived processes
    PCB *preempted_pcb = NULL;    // at most one per tick

    int now;
    int prev_time = 0;

    while (1) {
        now = getClk();
        if (now == prev_time) continue;  // TICK GATE
        prev_time = now;
        if(switching > 0)
            switching--;
        if(requesting_ram > 0)
            requesting_ram--;

        printf("S: time is %d\n", now);

        preempted_pcb = NULL;   // reset staging each tick

        // ── sync with generator ───────────────────────────────
        if (generator_sent_all == 0)
            sem_wait(sem_id, 0);

        samir:;
        // ── check for memory request from running process ─────
        // Must happen BEFORE ticking so the RAM access 1-tick cost
        // is counted as part of this tick (edit 1).
        // if(requesting_ram == 0) {
        //     requesting_ram = -1;
        //     goto ram_response;
        // }

        // printf("safe -3\n");
        printf("requesting ram = %d\n", requesting_ram);

        if (requesting_ram == 0 || (running_PCB != NULL && running_process_sem_id != -1)) {
            if(requesting_ram == -1 && dont_wait == 0) {
                printf("S: waiting\n");
                sem_wait(running_process_sem_id, 1);
                printf("S: awake\n");
            }
            struct mem_request req;
            if (requesting_ram == 0 || msgrcv(mem_req_qid, &req, sizeof(req) - sizeof(long), running_PCB->proc->ID, IPC_NOWAIT) != -1) {
                if(requesting_ram == -1) {
                    printf("S: memory request is received\n");
                    // ── EDIT 1: RAM access costs 1 tick ──────────
                    // tick the process once for the RAM trip
                    // remainingForProcess--;
                    // quantum_remaining--;
                    // ─────────────────────────────────────────────
    
                    requesting_ram = RAM_ACCESS_TIME;
                    goto reset_check;
                }

                requesting_ram = -1;

                int phys = MMU_translate(running_PCB, req.virtual_address, req.rw);
                if (phys == -1) {
                    // PAGE FAULT

                    // ── EDIT 3: log "stopped" before blocking ─
                    // update remaining time first so the log is accurate
                    running_PCB->remainingTime = remainingForProcess;
                    log_event(out, now, "stopped", running_PCB, 0);
                    // ─────────────────────────────────────────

                    MMU_handle_fault(mem_log, running_PCB, req.virtual_address, req.rw, now);

                    running_PCB->state = BLOCKED;
                    enqueue_blocked(running_PCB);

                    running_PCB            = NULL;
                    running_process_sem_id = -1;
                    isBlocked             = 1;
                    quantum_remaining      = quantum;
                    quantums_elapsed++;

                    // context-switch overhead still applies
                    // while (getClk() - now < CONTEXT_SWITCH_TIME);
                    // now       = getClk();
                    // prev_time = now;
                    // switching = 1;

                    // goto check_blocked;
                } else {
                    printf("S: memory request is not faulty\n");
                    isGranted = 1;
                    // no fault — send immediate response to process
                    struct mem_response res;
                    res.mtype   = running_PCB->proc->ID;
                    res.granted = 1;
                    msgsnd(mem_res_qid, &res, sizeof(res) - sizeof(long), 0);
                }
            }
        }

        if(isSamir > 0)
            goto reset_check;

        tick_process:;
        // ── tick the running process ──────────────────────────
        if (running_process_sem_id != -1) {
            printf("S: decrementing process %d remaining time\n", running_PCB->proc->ID);
            // if(requesting_ram == -1) 
            if(!isGranted) {
                printf("S: releasing sem for process at ticking\n");
                sem_release(running_process_sem_id, 0);
                isGranted = 0;
                dont_wait = 0;
            }
            remainingForProcess--;
            quantum_remaining--;

            if (remainingForProcess == 0) {
                sem_wait(running_process_sem_id, 1);
                running_PCB->remainingTime = 0;
                log_event(out, now, "finished", running_PCB, 1);

                // free all memory owned by this process
                MMU_process_finish(running_PCB);

                int turnaround_time = now - running_PCB->proc->arrivalTime;
                int waiting_time    = turnaround_time - running_PCB->proc->runTime;
                float wta = (running_PCB->proc->runTime > 0)
                    ? ((float)turnaround_time / running_PCB->proc->runTime)
                    : 0.0f;
                wta = ((int)(wta * 100.0f + 0.5f)) / 100.0f;

                total_run_time  += running_PCB->proc->runTime;
                sum_Wait        += waiting_time;
                sum_WTA         += wta;
                sum_WTA_squared += wta * wta;
                finished_processes_count++;

                free(running_PCB->proc);
                free(running_PCB);
                running_PCB            = NULL;
                semctl(running_process_sem_id, 0, IPC_RMID);
                running_process_sem_id = -1;
                isFinished             = 1;
                quantum_remaining      = quantum;
                quantums_elapsed++;
            } else {
                isFinished = 0;
            }
        }

        reset_check:;

        if(isSamir > 0) {
            isSamir = 0;
            goto afterSamir;
        }

        // ── R-bit reset every K quantums ─────────────────────
        if (quantum_remaining == 0 && running_PCB != NULL)
            quantums_elapsed++;

        if (K > 0 && quantums_elapsed > 0 && quantums_elapsed % K == 0)
            MMU_reset_r_bits();

        // ── EDIT 2 STEP A: handle preemption FIRST ────────────
        // if quantum expired and there are other processes waiting,
        // stage the preempted process — it will be pushed to ready
        // queue BEFORE unblocked and newly arrived processes.
        // printf("safe -2\n");

        if(switching == 0) {
            switching  = -1;
            goto check_blocked;
        }
        if (isBlocked || isFinished || running_PCB != NULL && quantum_remaining == 0) {
            PCB *current_PCB = peekPcb(rr_head);
            if (current_PCB != NULL) {
                printf("S: quantum expired AND something else is waiting -> preempt (CS)\n");
                // quantum expired AND something else is waiting → preempt
                // dequeuePcb(&rr_head);   // dequeue next to run
                if(!isFinished && !isBlocked) {

                    running_PCB->remainingTime -= now - running_PCB->last_start_time;
                    log_event(out, now, "stopped", running_PCB, 0);
    
                    // stage the preempted process — don't enqueue yet
                    preempted_pcb = running_PCB;
    
                    running_PCB            = NULL;
                    running_process_sem_id = -1;
    
                    
                    // while (getClk() - now < CONTEXT_SWITCH_TIME);
                    // now       = getClk();
                    // prev_time = now;
                    // push preempted FIRST into ready queue
                    enqueuePcb(&rr_head, preempted_pcb);
                }
                
                switching = CONTEXT_SWITCH_TIME;

                // now start the next process that was dequeued
                // (current_PCB is already dequeued above)
                // goto start_process;
            } else {
                printf("quantum expired but nothing else → extend\n");
                // quantum expired but nothing else → extend
                quantum_remaining = quantum;
            }
            isFinished = 0;
            isBlocked = 0;
        }

        check_blocked:;
        
        // printf("safe -1\n");

        // ── EDIT 2 STEP B: unblocked processes pushed SECOND ──
        printf("S: blocked_count = %d\n", blocked_count);
        for (int i = 0; i < blocked_count; i++) {
            PCB *p = blocked_list[i];
            if (now < p->disk_done_at) continue;
            printf("S: disk finished — update page table via MMU and push process %d to ready queue\n", p->proc->ID);
            // disk finished — update page table via MMU
            MMU_complete_page_load(mem_log, p, now);

            // send response so process unblocks
            struct mem_response res;
            res.mtype   = p->proc->ID;
            res.granted = 1;
            msgsnd(mem_res_qid, &res, sizeof(res) - sizeof(long), 0);

            p->state = READY;
            // push unblocked to ready queue (after preempted, before arrived)
            enqueuePcb(&rr_head, p);

            // remove from blocked list
            blocked_list[i] = blocked_list[--blocked_count];
            i--;
        }

        // printf("safe 0\n");

        // ── EDIT 2 STEP C: newly arrived processes pushed LAST ─
        struct msbuf msg;
        while (msgrcv(msgq_id, &msg, sizeof(msg.proc), 0, IPC_NOWAIT) != -1) {
            if (msg.proc.ID == -1) {
                generator_sent_all = 1;
                break;
            }
            PCB *new_pcb              = (PCB *)malloc(sizeof(PCB));
            new_pcb->proc             = (Process *)malloc(sizeof(Process));
            *(new_pcb->proc)          = msg.proc;
            new_pcb->state            = READY;
            new_pcb->remainingTime    = msg.proc.runTime;
            new_pcb->last_start_time  = -1;
            new_pcb->page_table_frame = -1;
            new_pcb->page_table_index = -1;
            new_pcb->pending_frame    = -1;
            new_pcb->pending_vpn      = -1;
            new_pcb->disk_done_at     = -1;
            new_pcb->disk_ticks       = 0;
            new_pcb->fault_rw         = 0;

            process_scheduler_sem(msg.proc.ID);

            // push arrived LAST
            enqueuePcb(&rr_head, new_pcb);
            printf("S: process %d has arrived\n", new_pcb->proc->ID);
        }

        // printf("safe 1\n");

        if(switching > 0)
            continue;
        // ── scheduling decision ───────────────────────────────
        schedule:;
        {
            PCB *current_PCB = peekPcb(rr_head);

            if (current_PCB == NULL && running_PCB == NULL) {
                if (generator_sent_all == 1 && blocked_list_empty())
                    break;
                continue;
            }

            if (running_PCB != NULL)
                continue;

            // CPU is free — start next from queue
            if (current_PCB == NULL) continue;
            dequeuePcb(&rr_head);

            // ── start or resume current_PCB ───────────────────
            if (current_PCB->last_start_time == -1) {
                // first time — allocate page table and load first page
                MMU_process_start(mem_log, current_PCB, now);

                pid_t pid = fork();
                if (pid == 0) {
                    char rt[20], semarg[20], idarg[20], reqarg[20], resarg[20];
                    sprintf(rt,     "%d", current_PCB->proc->runTime);
                    int s = semget(111 + current_PCB->proc->ID, 2, 0666);
                    sprintf(semarg, "%d", s);
                    sprintf(idarg,  "%d", current_PCB->proc->ID);
                    sprintf(reqarg, "%d", mem_req_qid);
                    sprintf(resarg, "%d", mem_res_qid);
                    execl("./process.out", "process.out",
                          rt, semarg, idarg, reqarg, resarg, NULL);
                    exit(1);
                }
                current_PCB->proc->PID = pid;
                log_event(out, now, "started", current_PCB, 0);
                isSamir = 1;
            } else {
                log_event(out, now, "resumed", current_PCB, 0);
                isSamir = 2;
            }
            
            current_PCB->last_start_time   = now;
            running_PCB                    = current_PCB;
            remainingForProcess            = running_PCB->remainingTime;
            quantum_remaining              = quantum;
            running_process_sem_id = semget(111 + running_PCB->proc->ID, 2, 0666);
            
            // if(isSamir == 1) {
            //     // sem_release(running_process_sem_id, 0);
            //     // printf("S: released sem at samir\n");
            // }
            
            if(isSamir == 2)
                dont_wait = 1;
            
            isGranted = 0;
            goto samir;
            
            afterSamir:;
        }
    }

    // ── write perf file ───────────────────────────────────────
    FILE *perf_out = fopen("scheduler.perf", "w");
    if (perf_out) {
        float util = (now > 0)
            ? (100.0f * total_run_time / now) : 0.0f;
        util = ((int)(util * 100.0f + 0.5f)) / 100.0f;

        float avg_WTA = (finished_processes_count > 0)
            ? sum_WTA / finished_processes_count : 0.0f;
        avg_WTA = ((int)(avg_WTA * 100.0f + 0.5f)) / 100.0f;

        float avg_wait = (finished_processes_count > 0)
            ? (float)sum_Wait / finished_processes_count : 0.0f;
        avg_wait = ((int)(avg_wait * 100.0f + 0.5f)) / 100.0f;

        float std_WTA = 0.0f;
        if (finished_processes_count > 0) {
            float var = (sum_WTA_squared / finished_processes_count)
                      - (avg_WTA * avg_WTA);
            std_WTA = (var > 0) ? sqrtf(var) : 0.0f;
            std_WTA = ((int)(std_WTA * 100.0f + 0.5f)) / 100.0f;
        }

        fprintf(perf_out, "CPU utilization = %.2f%%\n", util);
        fprintf(perf_out, "Avg WTA = %.2f\n", avg_WTA);
        fprintf(perf_out, "Avg Waiting = %.2f\n", avg_wait);
        fprintf(perf_out, "Std WTA = %.2f\n", std_WTA);
        fclose(perf_out);
    }

    fclose(out);
    fclose(mem_log);
}

// ─────────────────────────────────────────────────────────────
//  MAIN
// ─────────────────────────────────────────────────────────────
int main(int argc, char *argv[])
{
    initClk();

    // create memory request/response queues
    mem_req_qid = msgget(222, 0666 | IPC_CREAT);
    mem_res_qid = msgget(333, 0666 | IPC_CREAT);

    if (mem_req_qid == -1 || mem_res_qid == -1) {
        perror("memory msgget failed");
        exit(1);
    }

    msgq_id = msgget(111, 0666);
    if (msgq_id == -1) { perror("msgget failed"); exit(1); }

    sem_id = semget(111, 1, 0666);
    if (sem_id == -1) { perror("semget failed"); exit(1); }

    switch (atoi(argv[1])) {
        // case 1:
        //     HPF_DEMO();
        //     break;

        case 2:
            RR_DEMO(atoi(argv[2]), atoi(argv[3]));
            break;

        // case 3:
        //     MULTI_CPU_DEMO(atoi(argv[2]), atoi(argv[3]));
        //     break;

        default:
            fprintf(stderr, "Unknown algorithm\n");
            exit(1);
    }

    // cleanup memory queues
    msgctl(mem_req_qid, IPC_RMID, NULL);
    msgctl(mem_res_qid, IPC_RMID, NULL);

    destroyClk(true);
    return 0;
}