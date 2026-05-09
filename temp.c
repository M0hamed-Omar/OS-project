#include "scheduler.h"
#include <math.h>

int Schedulers_shm_id;
int Schedulers_sems_id;
shared_data *shm_ptr_2CPU;

int main(int argc, char *argv[])
{
    initClk();
    
    msgq_id = msgget(111, 0666);
    
    if (msgq_id == -1)
    {
        perror("msgget failed\n");
        exit(1);
    }
    sem_id = semget(111, 1, 0666);
    if(sem_id == -1) {
        perror("semget failed\n");
        exit(1);
    }

    switch(atoi(argv[1])) {
        case 1:  
            HPF_DEMO();
            break;

        case 2:
            RR_DEMO(atoi(argv[2]));
            break;

        case 3:
            MULTI_CPU_DEMO(atoi(argv[2]), atoi(argv[3]));
            break;
    }
    destroyClk(true);
    return 0;
}

void log_event(FILE *fp,int curr_time, char *state, PCB *p, int finished)
{
    int waiting = (curr_time - p->proc->arrivalTime) - (p->proc->runTime - p->remainingTime);
    if (finished == 1)
    {
        int TA = curr_time - p->proc->arrivalTime;
        float WTA = (float)TA / p->proc->runTime;

        fprintf(fp,
                "At time %d process %d %s arr %d total %d remain %d wait %d TA %d WTA %.2f\n",
                curr_time, p->proc->ID, state, p->proc->arrivalTime, p->proc->runTime,
                p->remainingTime, waiting, TA, WTA);
    }
    else
        fprintf(fp,
                "At time %d process %d %s arr %d total %d remain %d wait %d\n",
                curr_time, p->proc->ID, state, p->proc->arrivalTime, p->proc->runTime,
                p->remainingTime, waiting);
    fflush(fp);
}

void log_event_2CPU(FILE *fp,int curr_time, PCB2 *pcb, int status)
{
    int waiting = (curr_time - pcb->arrival_time) - (pcb->total_time - pcb->remaining_time);
    switch(status) {
        case 0:
        fprintf(fp,
            "At time %d process %d started arr %d total %d remain %d wait %d\n",
            curr_time, pcb->ID, pcb->arrival_time, pcb->total_time,
            pcb->remaining_time, waiting);
        break;

        case 1:
        int TA = curr_time - pcb->arrival_time;
        float WTA = (float)TA / pcb->total_time;

        fprintf(fp,
                "At time %d process %d finished arr %d total %d remain %d wait %d TA %d WTA %.2f\n",
                curr_time, pcb->ID, pcb->arrival_time, pcb->total_time,
                pcb->remaining_time, waiting, TA, WTA);
        break;
        case 2:
        fprintf(fp,
                "At time %d process %d was stolen\n",
                curr_time, pcb->ID, pcb->arrival_time, pcb->total_time,
                pcb->remaining_time, waiting, TA, WTA);
        break;
    }
    fflush(fp);
}

// void HPF_DEMO()
// {
//     FILE *out = fopen("scheduler.log", "w");
//     if (!out)
//     {
//         printf("File error\n");
//         exit(1);
//     }

//     PriNode *pri_queue;
//     initPriQueue(&pri_queue);
//     int remainingForProcess;
//     int switching = 0;
//     int running_process_sem_id = -1;
//     int isFinished = 0;
//     int generator_sent_all = 0;
    
//     int finished_processes_count = 0;
//     int total_run_time = 0;
//     float sum_WTA = 0.0f;
//     float sum_WTA_squared = 0.0f;
//     int sum_Wait = 0;

//     int now;
//     int prev_time = 0;
//     while (1)
//     {
//         now = getClk();
//         if (now == prev_time) continue;  // TICK GATE
//         prev_time = now;

        
//         if(generator_sent_all == 0)
//             sem_wait(sem_id, 0);
        

//         if(running_process_sem_id != -1) {
//             sem_release(running_process_sem_id, 0);
//             remainingForProcess--;
//             if(remainingForProcess == 0){
//                 sem_wait(running_process_sem_id, 1);
//                 running_PCB->remainingTime = 0;
//                 log_event(out, now, "finished", running_PCB, 1);

//                 int turnaround_time = now - running_PCB->proc->arrivalTime;
//                 int waiting_time = turnaround_time - running_PCB->proc->runTime;
//                 float wta = (running_PCB->proc->runTime > 0) ? ((float)turnaround_time / running_PCB->proc->runTime) : 0.0f;
//                 wta = ((int)(wta * 100.0f + 0.5f)) / 100.0f;
                
//                 total_run_time += running_PCB->proc->runTime;
//                 sum_Wait += waiting_time;
//                 sum_WTA += wta;
//                 sum_WTA_squared += (wta * wta);
//                 finished_processes_count++;
                
//                 free(running_PCB->proc);
//                 free(running_PCB);
//                 running_PCB = NULL;
//                 semctl(running_process_sem_id, 0, IPC_RMID);
//                 running_process_sem_id = -1;
//                 isFinished = 1;   
//             }
//         }

//         /* 1. RECEIVE NEW PROCESSES (FROM GENERATOR) */
//         struct msbuf msg;
//         while (msgrcv(msgq_id, &msg, sizeof(msg.proc), 0, IPC_NOWAIT) != -1)
//         {
//             if(msg.proc.ID == -1) {
//                 generator_sent_all = 1;
//                 break;
//             }
//             PCB *new_pcb = (PCB *)malloc(sizeof(PCB));
//             new_pcb->proc = (Process *)malloc(sizeof(Process));
//             *(new_pcb->proc) = msg.proc;
//             new_pcb->state = READY;
//             new_pcb->remainingTime = msg.proc.runTime;
//             new_pcb->last_start_time = -1;
//             process_scheduler_sem(msg.proc.ID);
//             enqueuePri(&pri_queue, new_pcb);
//         }

//         PCB *current_PCB = peekPri(pri_queue);
        
//         if (current_PCB == NULL) {
//             if(running_PCB == NULL && generator_sent_all == 1)
//                 break;
//             continue;
//         }
        
//         /* 2. PREEMPTION CHECK / SCHEDULING DECISION */
//         if (running_PCB == NULL)
//         {
//             if (isFinished && !switching)
//             {
//                 while (getClk() - now < CONTEXT_SWITCH_TIME);
//                 now = getClk();
//                 switching = 1;
//                 isFinished = 0;
//                 continue;
//             }
//         }
//         else
//         {
//             if (running_PCB->proc->priority <= current_PCB->proc->priority)
//                 continue;
//             if(!switching) {
//                 running_PCB->remainingTime -= (now - running_PCB->last_start_time);
//                 log_event(out, now, "stopped", running_PCB, 0);
//                 enqueuePri(&pri_queue, running_PCB);
    
//                 while (getClk() - now < CONTEXT_SWITCH_TIME);
//                 now = getClk();
//                 switching = 1;
//                 continue;
//             }
//         }

//         dequeuePri(&pri_queue);

//         /* 3. START OR RESUME current_PCB */
//         if (current_PCB->last_start_time == -1)
//         {
//             // first time running → fork
//             pid_t pid = fork();
//             if (pid == 0)
//             {
//                 char process_run_time[20];
//                 sprintf(process_run_time, "%d", current_PCB->proc->runTime);
//                 int sem_id_for_process = semget(111 + current_PCB->proc->ID, 2, 0666);
//                 char process_sem_id[20];
//                 sprintf(process_sem_id, "%d", sem_id_for_process);
//                 execl("./process.out", "process.out", process_run_time, process_sem_id, NULL);
//                 exit(1);
//             }
//             current_PCB->proc->PID = pid;
//             log_event(out, now, "started", current_PCB, 0);
//         }
//         else
//             log_event(out, now, "resumed", current_PCB, 0);

//         current_PCB->last_start_time = now;
//         running_PCB = current_PCB;
//         remainingForProcess =  running_PCB->remainingTime;  
//         running_process_sem_id = semget(111 + running_PCB->proc->ID, 2, 0666);
//         switching = 0;
//     }

//     FILE *perf_out = fopen("scheduler.perf", "w");
//     if (perf_out) {
//         float util = (now > 0) ? (100.0f * total_run_time / now) : 0.0f;
//         util = ((int)(util * 100.0f + 0.5f)) / 100.0f;
//         float avg_WTA = (finished_processes_count > 0) ? (sum_WTA / finished_processes_count) : 0.0f;
//         avg_WTA = ((int)(avg_WTA * 100.0f + 0.5f)) / 100.0f;
//         float avg_wait = (finished_processes_count > 0) ? ((float)sum_Wait / finished_processes_count) : 0.0f;
//         avg_wait = ((int)(avg_wait * 100.0f + 0.5f)) / 100.0f;
//         float std_WTA = 0.0f;
//         if (finished_processes_count > 0) {
//             float mean_sq = sum_WTA_squared / finished_processes_count;
//             float var = mean_sq - (avg_WTA * avg_WTA);
//             std_WTA = (var > 0) ? sqrtf(var) : 0.0f;
//             std_WTA = ((int)(std_WTA * 100.0f + 0.5f)) / 100.0f;
//         }
//         fprintf(perf_out, "CPU utilization = %.2f%%\n", util);
//         fprintf(perf_out, "Avg WTA = %.2f\n", avg_WTA);
//         fprintf(perf_out, "Avg Waiting = %.2f\n", avg_wait);
//         fprintf(perf_out, "Std WTA = %.2f\n", std_WTA);
//         fclose(perf_out);
//         fclose(out);
//     }
// }

void RR_DEMO(int quantum) {
    FILE *out = fopen("scheduler.log", "w");
    if (!out)
    {
        printf("File error\n");
        exit(1);
    }
    running_PCB = NULL;  

    PCBNode *rr_head;
    initPcbQueue(&rr_head);
    int quantum_remaining = quantum;
    int generator_sent_all = 0;
    int remainingForProcess = 0;
    int switching = 0;
    int running_process_sem_id = -1;
    int isFinished = 0;

    int finished_processes_count = 0;
    int total_run_time = 0;
    float sum_WTA = 0.0f;
    float sum_WTA_squared = 0.0f;
    int sum_Wait = 0;
    
    int now;
    int prev_time = 0;
    while (1) {
        now = getClk();
        if (now == prev_time) continue;  // TICK GATE
        prev_time = now;


        // sync with generator
        if (generator_sent_all == 0)
            sem_wait(sem_id, 0);

        // tick the running process and check if it finished
        if (running_process_sem_id != -1) {
            sem_release(running_process_sem_id, 0);
            remainingForProcess--;
            quantum_remaining--;

            if (remainingForProcess == 0) {
                // process is done — wait for it to destroy its semaphore
                sem_wait(running_process_sem_id, 1);
                running_PCB->remainingTime = 0;
                log_event(out, now, "finished", running_PCB, 1);

                int turnaround_time = now - running_PCB->proc->arrivalTime;
                int waiting_time = turnaround_time - running_PCB->proc->runTime;
                float wta = (running_PCB->proc->runTime > 0) ? ((float)turnaround_time / running_PCB->proc->runTime) : 0.0f;
                wta = ((int)(wta * 100.0f + 0.5f)) / 100.0f;
                
                total_run_time += running_PCB->proc->runTime;
                sum_Wait += waiting_time;
                sum_WTA += wta;
                sum_WTA_squared += (wta * wta);
                finished_processes_count++;

                free(running_PCB->proc);
                free(running_PCB);
                running_PCB = NULL;
                semctl(running_process_sem_id, 0, IPC_RMID);
                running_process_sem_id = -1;
                isFinished = 1;
                quantum_remaining = quantum;
            } else {
                isFinished = 0;
            }
        }

        // receive new arrivals
        int empty_before_receiving = isEmptyPcb(rr_head);
        int only_once = 1;
        struct msbuf msg;
        while (msgrcv(msgq_id, &msg, sizeof(msg.proc), 0, IPC_NOWAIT) != -1) {
            if(msg.proc.ID == -1) {
                generator_sent_all = 1;
                break;
            }
            PCB *new_pcb = (PCB *)malloc(sizeof(PCB));
            new_pcb->proc = (Process *)malloc(sizeof(Process));
            *(new_pcb->proc) = msg.proc;
            new_pcb->state = READY;
            new_pcb->remainingTime = msg.proc.runTime;
            new_pcb->last_start_time = -1;
            process_scheduler_sem(msg.proc.ID);
            if(quantum_remaining == 0 && running_PCB != NULL && only_once == 1 && empty_before_receiving == 0) {
                enqueuePcb(&rr_head, running_PCB);
                only_once = 0;
            }
            enqueuePcb(&rr_head, new_pcb);
        }

        if(empty_before_receiving == 1 && quantum_remaining == 0) {
            quantum_remaining = quantum;
            empty_before_receiving = 0;
        }
        // scheduling decision
        PCB *current_PCB = peekPcb(rr_head);

        if (current_PCB == NULL && running_PCB == NULL) {
            if(generator_sent_all == 1)
                break;
            continue;
        }

        if (running_PCB != NULL) {
            // quantum not expired yet → keep running
            if (quantum_remaining > 0)
                continue;

            // quantum expired but nothing else to run → extend
            if (current_PCB == NULL) {
                quantum_remaining = quantum;
                continue;
            }

            // quantum expired AND there is something else → preempt
            dequeuePcb(&rr_head);

            // stop the running process via its semaphore
            // do NOT release sem[0] again — just stop signaling it
            running_PCB->remainingTime -= now - running_PCB->last_start_time;
            log_event(out, now, "stopped", running_PCB, 0);
            
            if(only_once == 1)
                enqueuePcb(&rr_head, running_PCB);

            // context switch overhead
            while (getClk() - now < CONTEXT_SWITCH_TIME);
            now = getClk();
            switching = 1;

        } else {
            // CPU is free
            dequeuePcb(&rr_head);

            if (isFinished) {
                while (getClk() - now < CONTEXT_SWITCH_TIME);
                now = getClk();
                switching = 1;
            }
        }

        /* START OR RESUME current_PCB */
        if (current_PCB->last_start_time == -1) {
            pid_t pid = fork();
            if (pid == 0) {
                char process_run_time[20];
                sprintf(process_run_time, "%d", current_PCB->proc->runTime);
                int sem_id_for_process = semget(111 + current_PCB->proc->ID, 2, 0666);
                char process_sem_id[20];
                sprintf(process_sem_id, "%d", sem_id_for_process);
                execl("./process.out", "process.out", process_run_time, process_sem_id, NULL);
                exit(1);
            }
            current_PCB->proc->PID = pid;
            log_event(out, now, "started", current_PCB, 0);
        } else {
            log_event(out, now, "resumed", current_PCB, 0);
        }

        current_PCB->last_start_time = now;
        running_PCB = current_PCB;
        remainingForProcess = running_PCB->remainingTime + switching;
        quantum_remaining = quantum + switching;
        running_process_sem_id = semget(111 + running_PCB->proc->ID, 2, 0666);

        // if we just switched, release the new process's sem[0] for this tick
        // sem_release(running_process_sem_id, 0);
        switching = 0;
    }
    FILE *perf_out = fopen("scheduler.perf", "w");
    if (perf_out) {
        float util = (now > 0) ? (100.0f * total_run_time / now) : 0.0f;
        util = ((int)(util * 100.0f + 0.5f)) / 100.0f;
        float avg_WTA = (finished_processes_count > 0) ? (sum_WTA / finished_processes_count) : 0.0f;
        avg_WTA = ((int)(avg_WTA * 100.0f + 0.5f)) / 100.0f;
        float avg_wait = (finished_processes_count > 0) ? ((float)sum_Wait / finished_processes_count) : 0.0f;
        avg_wait = ((int)(avg_wait * 100.0f + 0.5f)) / 100.0f;
        float std_WTA = 0.0f;
        if (finished_processes_count > 0) {
            float mean_sq = sum_WTA_squared / finished_processes_count;
            float var = mean_sq - (avg_WTA * avg_WTA);
            std_WTA = (var > 0) ? sqrtf(var) : 0.0f;
            std_WTA = ((int)(std_WTA * 100.0f + 0.5f)) / 100.0f;
        }
        fprintf(perf_out, "CPU utilization = %.2f%%\n", util);
        fprintf(perf_out, "Avg WTA = %.2f\n", avg_WTA);
        fprintf(perf_out, "Avg Waiting = %.2f\n", avg_wait);
        fprintf(perf_out, "Std WTA = %.2f\n", std_WTA);
        fclose(perf_out);
        fclose(out);
    }
}

// void MULTI_CPU_DEMO(int N, int M)
// {
    
//     Schedulers_shm_id = shmget(SHM_KEY_FOR_2CPU, sizeof(shared_data), IPC_CREAT | 0666);
//     if (Schedulers_shm_id == -1) {
//         perror("shmget failed");
//         exit(1);
//     }

//     shm_ptr_2CPU = (shared_data *) shmat(Schedulers_shm_id, NULL, 0);

//     shm_ptr_2CPU->done = 0;
//     shm_ptr_2CPU->turn = -1;
    

//     for (int i = 0; i < 2; i++) {
//         shm_ptr_2CPU->S[i].total_remaining = 0;
//         shm_ptr_2CPU->S[i].q_size = 0;
//     }

//     Schedulers_sems_id = semget(SEM_KEY_FOR_2CPU, 4, IPC_CREAT | 0666);
//     if (Schedulers_sems_id == -1) {
//         perror("semget failed");
//         exit(1);
//     }

//     for (int i = 0; i < 4; i++) 
//         semctl(Schedulers_sems_id, i, SETVAL, 0);

//     pid_t p1 = fork();

//     if (p1 == 0) {
//         cpu_worker(0, Schedulers_shm_id);
//         return 0;
//     }

//     pid_t p2 = fork();
//     if (p2 == 0) {
//         cpu_worker(1, Schedulers_shm_id);
//         return 0;
//     }

//     signal(SIGINT, clear_2CPU_resources);

//     int processes_received = 0;
//     int check_interval = N;
//     int stealing = 0;
//     int generator_sent_all = 0;
//     int stealing_recheck = 0;

//     int now;
//     int prev_time = 0;
//     while (1)
//     {
//         if(stealing_recheck == 0) {
//             now = getClk();
//             if (now == prev_time) continue;  // TICK GATE
//             prev_time = now;
            
//             if(generator_sent_all == 0)
//                 sem_wait(sem_id, 0);
        
//             check_interval--;
//             for(int i = 0; i < CPU_COUNT; i++) 
//                 sem_wait(Schedulers_sems_id, MANAGED);
                
//             int curr_finished = 0;
//             for(int i = 0; i < CPU_COUNT; i++)
//                 curr_finished += shm_ptr_2CPU->S[i].finished;
//             if (generator_sent_all == 1 && processes_received == curr_finished) {
//                 shm_ptr_2CPU->done = 1;
//                 for(int i = 0; i < CPU_COUNT; i++)
//                     sem_release(Schedulers_sems_id, MANAGER);
//                 break;
//             }
//             shm_ptr_2CPU->done = 0;
//             for(int i = 0; i < CPU_COUNT; i++)
//                 sem_release(Schedulers_sems_id, MANAGER);
//             for(int i = 0; i < CPU_COUNT; i++)
//                 sem_wait(Schedulers_sems_id, MANAGED);
//         }

//         while(1) {
//             struct msbuf msg;
//             if (msgrcv(msgq_id, &msg, sizeof(msg.proc), 0, IPC_NOWAIT) != -1)
//             {
//                 if(msg.proc.ID == -1) {
//                     shm_ptr_2CPU->done = 1;
//                     generator_sent_all = 1;
//                     for(int i = 0; i < CPU_COUNT; i++)
//                         sem_release(Schedulers_sems_id, MANAGER);
//                     for(int i = 0; i < CPU_COUNT; i++)
//                         sem_wait(Schedulers_sems_id, MANAGED);
//                     break;
//                 }
//                 shm_ptr_2CPU->done = 0;
//                 for(int i = 0; i < CPU_COUNT; i++)
//                     sem_release(Schedulers_sems_id, MANAGER);
//                 for(int i = 0; i < CPU_COUNT; i++)
//                     sem_wait(Schedulers_sems_id, MANAGED);
//                 processes_received++;
//                 shm_ptr_2CPU->new_pcb.arrival_time = msg.proc.arrivalTime;
//                 shm_ptr_2CPU->new_pcb.total_time = msg.proc.runTime;
//                 shm_ptr_2CPU->new_pcb.remaining_time = msg.proc.runTime;
//                 shm_ptr_2CPU->new_pcb.ID = msg.proc.ID;
//                 if(shm_ptr_2CPU->S[0].q_size > shm_ptr_2CPU->S[1].q_size)
//                     shm_ptr_2CPU->turn = 1;
//                 else
//                     shm_ptr_2CPU->turn = 0;

//                 for(int i = 0; i < CPU_COUNT; i++)
//                     sem_release(Schedulers_sems_id, MANAGER2);
//                 for(int i = 0; i < CPU_COUNT; i++)
//                     sem_wait(Schedulers_sems_id, MANAGED);
//             }
//             else {
//                 shm_ptr_2CPU->done = 1;
//                 for(int i = 0; i < CPU_COUNT; i++)
//                     sem_release(Schedulers_sems_id, MANAGER);
//                 for(int i = 0; i < CPU_COUNT; i++)
//                     sem_wait(Schedulers_sems_id, MANAGED);
//                 break;
//             }
//         }
        
//         if (check_interval == 0 && abs(shm_ptr_2CPU->S[0].total_remaining - shm_ptr_2CPU->S[1].total_remaining) > M)
//         {
//             int t = (shm_ptr_2CPU->S[0].total_remaining > shm_ptr_2CPU->S[1].total_remaining) ? 1 : 0;
//             if((t == 1 && shm_ptr_2CPU->S[0].q_size == 0) || (t == 0 && shm_ptr_2CPU->S[1].q_size == 0)) {
//                 shm_ptr_2CPU->done = 1;
//                 for(int i = 0; i < CPU_COUNT; i++)
//                     sem_release(Schedulers_sems_id, MANAGER);
//                 for(int i = 0; i < CPU_COUNT; i++)
//                     sem_wait(Schedulers_sems_id, MANAGED);
//                 stealing_recheck = 0;
//             } 
//             else {
//                 shm_ptr_2CPU->done = 0;
//                 for(int i = 0; i < CPU_COUNT; i++)
//                     sem_release(Schedulers_sems_id, MANAGER);
//                 for(int i = 0; i < CPU_COUNT; i++)
//                     sem_wait(Schedulers_sems_id, MANAGED);
                
//                 shm_ptr_2CPU->turn = t;
    
//                 //simulate stealing
//                 while (getClk() - now < STEAL_OVERHEAD);
//                 now = getClk();
//                 stealing = 1;
    
//                 for(int i = 0; i < CPU_COUNT; i++)
//                     sem_release(Schedulers_sems_id, MANAGER2);
//                 for(int i = 0; i < CPU_COUNT; i++)
//                     sem_wait(Schedulers_sems_id, MANAGED);
//                 stealing_recheck = 1;
//                 continue;
//             }
//         }
//         else {
//             shm_ptr_2CPU->done = 1;
//             for(int i = 0; i < CPU_COUNT; i++)
//                 sem_release(Schedulers_sems_id, MANAGER);
//             for(int i = 0; i < CPU_COUNT; i++)
//                 sem_wait(Schedulers_sems_id, MANAGED);
//             stealing_recheck = 0;
//         }

//         if(check_interval == 0) {
//             check_interval = N + stealing;
//             stealing = 0;
//         }
//     }

    
//     wait(NULL);
//     wait(NULL);

//     // cleanup
//     shmdt(shm_ptr_2CPU);
//     shmctl(Schedulers_shm_id, IPC_RMID, NULL);
//     semctl(Schedulers_sems_id, 0, IPC_RMID);
// }

// void cpu_worker(int cpu_id, int Schedulers_shm_id)
// {
//     int local_sems = semget(SEM_KEY_FOR_2CPU, 4, 0666);
//     if (local_sems == -1) {
//         perror("semget failed in child");
//         exit(1);
//     }

//     shared_data *local_shm = (shared_data *) shmat(Schedulers_shm_id, NULL, 0);
//     if (local_shm == (void *) -1) {
//         perror("shmat failed in child");
//         exit(1);
//     }
//     FILE *out;
//     PCB2 *running_PCB = NULL;  

//     if(cpu_id == 0)
//         out = fopen("scheduler_1.log", "w");
//     else
//         out = fopen("scheduler_2.log", "w");
//     if (!out) { printf("File error\n"); exit(1); }

//     signal(SIGINT, clear_2CPU_resources_managed);
//     PCB2_Deque *deque = (PCB2_Deque *)malloc(sizeof(PCB2_Deque));
//     initDeque(deque);
//     int running_process_sem_id = -1;
//     int remainingForProcess;
//     int switching = 0;
//     int isFinished = 0;
//     int stealing = 0;
//     int stealing_recheck = 0;

//     // ── PERF ACCUMULATORS ──────────────────────────────
//     int finished_processes_count = 0;
//     int total_run_time = 0;
//     float sum_WTA = 0.0f;
//     float sum_WTA_squared = 0.0f;
//     int sum_Wait = 0;
//     int first_start_time = -1;
//     int last_finish_time = 0;
//     // ───────────────────────────────────────────────────
    
//     int now;
//     int prev_time = 0;
//     while (1)
//     {
//         if(stealing_recheck == 0) {
//             now = getClk();
//             if (now == prev_time) continue;
//             prev_time = now;

//             if(running_process_sem_id != -1) {
//                 sem_release(running_process_sem_id, 0);
//                 remainingForProcess--;
//                 local_shm->S[cpu_id].total_remaining--;
//                 if(remainingForProcess == 0){
//                     sem_wait(running_process_sem_id, 1);
//                     running_PCB->remaining_time = 0;
//                     log_event_2CPU(out, now, running_PCB, 1);
    
//                     // ── PERF: accumulate on finish ─────────────────
//                     int turnaround_time = now - running_PCB->arrival_time;
//                     int waiting_time = turnaround_time - running_PCB->total_time;
//                     float wta = (running_PCB->total_time > 0)
//                         ? ((float)turnaround_time / running_PCB->total_time)
//                         : 0.0f;
    
//                     total_run_time += running_PCB->total_time;
//                     sum_Wait += waiting_time;
//                     sum_WTA += wta;
//                     sum_WTA_squared += wta * wta;
//                     finished_processes_count++;
//                     last_finish_time = now;
//                     // ───────────────────────────────────────────────
    
//                     free(running_PCB);
//                     running_PCB = NULL;
//                     semctl(running_process_sem_id, 0, IPC_RMID);
//                     running_process_sem_id = -1;
//                     isFinished = 1;
//                     local_shm->S[cpu_id].finished++;   
//                 }    
//             }
//             local_shm->S[cpu_id].q_size = getSize(deque);
    
//             sem_release(local_sems, MANAGED);
    
//             sem_wait(local_sems, MANAGER);
//             if(local_shm->done == 1)
//                 break;
//             sem_release(local_sems, MANAGED);
//         }
            
//         while(1) {
//             sem_wait(local_sems, MANAGER);
//             if(local_shm->done == 1) {
//                 sem_release(local_sems, MANAGED);
//                 break;
//             }
//             sem_release(local_sems, MANAGED);
//             sem_wait(local_sems, MANAGER2);
//             if(local_shm->turn == cpu_id) {
//                 PCB2 *new_proc = (PCB2 *)malloc(sizeof(PCB2));
//                 *new_proc = local_shm->new_pcb;
//                 new_proc->sem_id = process_scheduler_sem(new_proc->ID);
//                 pushBack(deque, new_proc);
//                 local_shm->S[cpu_id].total_remaining += new_proc->remaining_time;
//                 local_shm->S[cpu_id].last_PCB = *peekBack(deque);
//                 local_shm->S[cpu_id].q_size = getSize(deque);
//             }
//             sem_release(local_sems, MANAGED);
//         }
        
        
//         sem_wait(local_sems, MANAGER);
//         if(local_shm->done == 1) {
//             sem_release(local_sems, MANAGED);
//             stealing_recheck = 0;
//         }
//         else
//         {
//             sem_release(local_sems, MANAGED);
//             prev_time = now;
//             sem_wait(local_sems, MANAGER2);
//             stealing = 1;
//             now = getClk();
//             int N = now - prev_time;
//             if(local_shm->turn == cpu_id) {
//                 PCB2 *new_proc = (PCB2 *)malloc(sizeof(PCB2));
//                 *new_proc = local_shm->S[1 - cpu_id].last_PCB;
//                 new_proc->sem_id = process_scheduler_sem(new_proc->ID);
//                 pushBack(deque, new_proc);
//                 local_shm->S[cpu_id].total_remaining += new_proc->remaining_time;
//                 local_shm->S[cpu_id].last_PCB = *new_proc;
//                 sem_release(local_sems, MANAGED2);
//             }
//             else {
//                 sem_wait(local_sems, MANAGED2);
//                 PCB2 *stolen = popBack(deque);
//                 local_shm->S[cpu_id].total_remaining -= stolen->remaining_time;
//                 log_event_2CPU(out, now - N, stolen, 2);
//                 if (!isEmpty(deque))
//                 local_shm->S[cpu_id].last_PCB = *peekBack(deque);
//             }
//             local_shm->S[cpu_id].q_size = getSize(deque);
//             sem_release(local_sems, MANAGED);
//             stealing_recheck = 1;
//             continue;
//         }
        


//         if(running_process_sem_id == -1) {
//             if(!isEmpty(deque)) {
//                 if(isFinished && stealing) {
//                     stealing = 0;
//                     continue;
//                 }
//                 if (isFinished)
//                 {
//                     while (getClk() - now < CONTEXT_SWITCH_TIME);
//                     now = getClk();
//                     switching = 1;
//                     isFinished = 0;
//                 }
//                 PCB2 *curr = popFront(deque);
//                 remainingForProcess = curr->remaining_time + switching;
//                 running_process_sem_id = curr->sem_id;
//                 running_PCB = curr;

//                 pid_t pid = fork();
//                 if (pid == 0)
//                 {
//                     char process_run_time[20];
//                     sprintf(process_run_time, "%d", curr->total_time);
//                     char sem_id_arg[20];
//                     sprintf(sem_id_arg, "%d", curr->sem_id);
//                     execl("./process.out", "process.out", process_run_time, sem_id_arg, NULL);
//                     exit(1);
//                 }
//                 log_event_2CPU(out, now, curr, 0);
//                 switching = 0;
//             }
//             isFinished = 0;
//             local_shm->S[cpu_id].q_size = getSize(deque);
//         }
//         else {
//             remainingForProcess += stealing;
//             stealing = 0;
//             isFinished = 0;
//         }
//     }

//     // ── WRITE PERF FILE ────────────────────────────────────────────
//     char perf_filename[32];
//     sprintf(perf_filename, "scheduler_%d.perf", cpu_id + 1);
//     FILE *perf_out = fopen(perf_filename, "w");
//     first_start_time = 0;
//     if (perf_out) {
//         int elapsed = (first_start_time != -1)
//             ? (last_finish_time - first_start_time)
//             : 1;

//         float util = (elapsed > 0)
//             ? (100.0f * total_run_time / elapsed)
//             : 0.0f;

//         float avg_WTA = (finished_processes_count > 0)
//             ? (sum_WTA / finished_processes_count)
//             : 0.0f;

//         float avg_wait = (finished_processes_count > 0)
//             ? ((float)sum_Wait / finished_processes_count)
//             : 0.0f;

//         float std_WTA = 0.0f;
//         if (finished_processes_count > 0) {
//             float mean_sq = sum_WTA_squared / finished_processes_count;
//             float sq_mean = avg_WTA * avg_WTA;
//             float var = mean_sq - sq_mean;
//             std_WTA = (var > 0) ? sqrtf(var) : 0.0f;
//         }

//         fprintf(perf_out, "CPU utilization = %.2f%%\n", util);
//         fprintf(perf_out, "Avg WTA = %.2f\n", avg_WTA);
//         fprintf(perf_out, "Avg Waiting = %.2f\n", avg_wait);
//         fprintf(perf_out, "Std WTA = %.2f\n", std_WTA);
//         fflush(perf_out);
//         fclose(perf_out);
//         fclose(out);
//     }
//     // ───────────────────────────────────────────────────────────────

//     shmdt(local_shm);
//     exit(0);
// }

// void clear_2CPU_resources(int signum) {
//     wait(NULL);
//     wait(NULL);
//     // cleanup
//     shmdt(shm_ptr_2CPU);
//     shmctl(Schedulers_shm_id, IPC_RMID, NULL);
//     semctl(Schedulers_sems_id, 0, IPC_RMID);
//     destroyClk(true);
//     exit(0);
// }

// void clear_2CPU_resources_managed(int signum) {
//     // cleanup
//     shmdt(shm_ptr_2CPU);
    
//     exit(0);
// }


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////2020202

#include "scheduler.h"

// ─────────────────────────────────────────────────────────────
//  GLOBALS
// ─────────────────────────────────────────────────────────────
int msgq_id;
int sem_id;
int mmu_req_qid;   // scheduler → MMU
int mmu_res_qid;   // MMU → scheduler
int mem_req_qid;   // process  → scheduler (memory access requests)
int mem_res_qid;   // scheduler → process  (fault resolved)
PCB *running_PCB = NULL;

// blocked list — processes waiting for disk to finish
#define MAX_BLOCKED 64
static PCB *blocked_list[MAX_BLOCKED];
static int  blocked_count = 0;
static int  disk_free_at = 0;

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
//  MMU COMMUNICATION HELPERS
// ─────────────────────────────────────────────────────────────

// send a request to the MMU and wait for its response (blocking)
static struct mmu_response mmu_send(struct mmu_request *req) {
    req->mtype = 1;
    if (msgsnd(mmu_req_qid, req, sizeof(*req) - sizeof(long), 0) == -1)
        perror("mmu_send: msgsnd failed");

    
    struct mmu_response res;
    if (msgrcv(mmu_res_qid, &res, sizeof(res) - sizeof(long), 1, 0) == -1)
        perror("mmu_send: msgrcv failed");
    return res;
}

static struct mmu_response mmu_start_process(PCB *pcb, int now) {
    struct mmu_request req;
    memset(&req, 0, sizeof(req));
    req.req_type = MMU_MSG_START;
    req.pid      = pcb->proc->ID;
    req.base     = pcb->proc->base;
    req.limit    = pcb->proc->limit;
    req.now      = now;
    return mmu_send(&req);
}

static struct mmu_response mmu_access(PCB *pcb, int vaddr, char rw, int now) {
    struct mmu_request req;
    memset(&req, 0, sizeof(req));
    req.req_type         = MMU_MSG_ACCESS;
    req.pid              = pcb->proc->ID;
    req.virtual_address  = vaddr;
    req.rw               = rw;
    req.now              = now;

    return mmu_send(&req);
}

static struct mmu_response mmu_complete(PCB *pcb, int now) {
    struct mmu_request req;
    memset(&req, 0, sizeof(req));
    req.req_type         = MMU_MSG_COMPLETE;
    req.pid              = pcb->proc->ID;
    req.virtual_address  = pcb->pending_vpn;     // vpn
    req.physical_address = pcb->pending_frame;   // frame
    req.rw               = pcb->fault_rw;
    req.base             = pcb->proc->base;
    req.now              = now;
    return mmu_send(&req);
}

static struct mmu_response mmu_finish_process(PCB *pcb) {
    struct mmu_request req;
    memset(&req, 0, sizeof(req));
    req.req_type         = MMU_MSG_FINISH;
    req.pid              = pcb->proc->ID;
    req.physical_address = pcb->page_table_frame;
    return mmu_send(&req);
}

static struct mmu_response mmu_reset_r() {
    struct mmu_request req;
    memset(&req, 0, sizeof(req));
    req.req_type = MMU_MSG_RESET_R;
    return mmu_send(&req);
}

static void mmu_shutdown() {
    struct mmu_request req;
    memset(&req, 0, sizeof(req));
    req.req_type = MMU_MSG_SHUTDOWN;
    req.mtype    = 1;
    msgsnd(mmu_req_qid, &req, sizeof(req) - sizeof(long), 0);
}

// ─────────────────────────────────────────────────────────────
//  CHECK BLOCKED PROCESSES
// ─────────────────────────────────────────────────────────────
static void check_blocked_processes(PCBNode **rr_head, int now) {
    for (int i = 0; i < blocked_count; i++) {
        PCB *p = blocked_list[i];
        if (now < p->disk_done_at) continue;

        // disk finished — tell MMU to update the page table
        mmu_complete(p, now);

        // unblock the process: send response so it continues
        struct mem_response res;
        res.mtype   = p->proc->ID;
        res.granted = 1;
        msgsnd(mem_res_qid, &res, sizeof(res) - sizeof(long), 0);

        p->state = READY;
        enqueuePcb(rr_head, p);

        // remove from blocked list
        blocked_list[i] = blocked_list[--blocked_count];
        i--;
    }
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
    FILE *out = fopen("scheduler.log", "w");
    if (!out) { perror("scheduler.log open failed"); exit(1); }

    running_PCB = NULL;

    PCBNode *rr_head;
    initPcbQueue(&rr_head);

    int quantum_remaining      = quantum;
    int generator_sent_all     = 0;
    int remainingForProcess    = 0;
    int switching              = 0;
    int running_process_sem_id = -1;
    int isFinished             = 0;

    // perf accumulators
    int   finished_processes_count = 0;
    int   total_run_time           = 0;
    float sum_WTA                  = 0.0f;
    float sum_WTA_squared          = 0.0f;
    int   sum_Wait                 = 0;

    // K-quantum R-bit reset tracking
    int quantums_elapsed = 0;

    int now;
    int prev_time = 0;

    while (1) {
        now = getClk();
        if (now == prev_time) continue;  // TICK GATE
        prev_time = now;

        // ── sync with generator ───────────────────────────────
        if (generator_sent_all == 0)
            sem_wait(sem_id, 0);

        // ── check blocked processes (disk done?) ──────────────
        check_blocked_processes(&rr_head, now);

        // ── tick the running process ──────────────────────────
        if (running_process_sem_id != -1) {
            sem_release(running_process_sem_id, 0);
            remainingForProcess--;
            quantum_remaining--;

            if (remainingForProcess == 0) {
                sem_wait(running_process_sem_id, 1);
                running_PCB->remainingTime = 0;
                log_event(out, now, "finished", running_PCB, 1);

                // tell MMU to free all memory for this process
                mmu_finish_process(running_PCB);

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

        // ── check for memory request from running process ─────
        if (running_PCB != NULL && running_process_sem_id != -1) {
            struct mem_request req;
            if (msgrcv(mem_req_qid, &req, sizeof(req) - sizeof(long), running_PCB->proc->ID, IPC_NOWAIT) != -1) {
                // forward access to MMU
                //
                struct mmu_response mres = mmu_access(running_PCB, req.virtual_address, req.rw, now);

                while (getClk() - now < RAM_ACCESS_TIME);
                now       = getClk();
                prev_time = now;
                remainingForProcess--;
                quantum_remaining--;

                if (mres.res_type == MMU_RES_FAULT) {
                    // PAGE FAULT — block the process
                    running_PCB->pending_frame = mres.pending_frame;
                    running_PCB->pending_vpn   = mres.pending_vpn;
                    running_PCB->fault_rw      = req.rw;
                    running_PCB->disk_ticks    = mres.disk_ticks;
                    int disk_start = (now> disk_free_at) ? now : disk_free_at;
                    running_PCB->disk_done_at = disk_start + mres.disk_ticks;
                    disk_free_at = running_PCB->disk_done_at;
                    running_PCB->remainingTime -= now - running_PCB->last_start_time;
                    running_PCB->pending_frame = mres.pending_frame;
                    running_PCB->state         = BLOCKED;

                    log_event(out, now, "stopped", running_PCB, 0);
                    enqueue_blocked(running_PCB);
                    running_PCB            = NULL;
                    running_process_sem_id = -1;
                    isFinished             = 0;
                    quantum_remaining      = quantum;
                    quantums_elapsed++;

                    // context-switch overhead still applies
                    while (getClk() - now < CONTEXT_SWITCH_TIME);
                    now       = getClk();
                    prev_time = now;
                    switching = 1;

                    goto scheduling;
                } else {
                    // no fault — send immediate response to process
                    struct mem_response res;
                    res.mtype   = running_PCB->proc->ID;
                    res.granted = 1;
                    msgsnd(mem_res_qid, &res, sizeof(res) - sizeof(long), 0);
                }
            }
        }

        // ── R-bit reset every K quantums ─────────────────────
        if (quantum_remaining == 0 && running_PCB != NULL)
            quantums_elapsed++;

        if (K > 0 && quantums_elapsed > 0 && quantums_elapsed % K == 0) {
            mmu_reset_r();
        }

        // ── receive new arrivals ──────────────────────────────
        int empty_before_receiving = isEmptyPcb(rr_head);
        int only_once = 1;
        struct msbuf msg;
        while (msgrcv(msgq_id, &msg, sizeof(msg.proc), 0, IPC_NOWAIT) != -1) {
            if (msg.proc.ID == -1) { generator_sent_all = 1; break; }

            PCB *new_pcb             = (PCB *)malloc(sizeof(PCB));
            new_pcb->proc            = (Process *)malloc(sizeof(Process));
            *(new_pcb->proc)         = msg.proc;
            new_pcb->state           = READY;
            new_pcb->remainingTime   = msg.proc.runTime;
            new_pcb->last_start_time = -1;
            new_pcb->page_table_frame = -1;
            new_pcb->page_table_index = -1;
            new_pcb->pending_frame    = -1;
            new_pcb->pending_vpn      = -1;
            new_pcb->disk_done_at     = -1;
            new_pcb->disk_ticks       = 0;
            new_pcb->fault_rw         = 0;

            process_scheduler_sem(msg.proc.ID);

            if (quantum_remaining == 0 && running_PCB != NULL
                && only_once == 1 && empty_before_receiving == 0) {
                enqueuePcb(&rr_head, running_PCB);
                only_once = 0;
            }
            enqueuePcb(&rr_head, new_pcb);
        }

        if (empty_before_receiving == 1 && quantum_remaining == 0) {
            quantum_remaining    = quantum;
            empty_before_receiving = 0;
        }

        scheduling:;

        // ── scheduling decision ───────────────────────────────
        PCB *current_PCB = peekPcb(rr_head);

        if (current_PCB == NULL && running_PCB == NULL) {
            if (generator_sent_all == 1 && blocked_list_empty())
                break;
            isFinished = 0;
            continue;
        }

        if (running_PCB != NULL) {
            if (quantum_remaining > 0) continue;

            if (current_PCB == NULL) {
                quantum_remaining = quantum;
                continue;
            }

            // preempt
            dequeuePcb(&rr_head);
            running_PCB->remainingTime -= now - running_PCB->last_start_time;
            log_event(out, now, "stopped", running_PCB, 0);

            if (only_once == 1)
                enqueuePcb(&rr_head, running_PCB);

            while (getClk() - now < CONTEXT_SWITCH_TIME);
            now       = getClk();
            prev_time = now;
            switching = 1;

        } else {
            dequeuePcb(&rr_head);
            if (isFinished) {
                while (getClk() - now < CONTEXT_SWITCH_TIME);
                now       = getClk();
                prev_time = now;
                switching = 1;
            }
        }

        // ── start or resume current_PCB ───────────────────────
        if (current_PCB->last_start_time == -1) {
            // first time — ask MMU to set up page table + load page 0
            struct mmu_response mres = mmu_start_process(current_PCB, now);
            current_PCB->page_table_frame = mres.pt_frame;

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
        } else {
            log_event(out, now, "resumed", current_PCB, 0);
        }

        current_PCB->last_start_time   = now;
        running_PCB                    = current_PCB;
        remainingForProcess            = running_PCB->remainingTime + switching;
        quantum_remaining              = quantum + switching;
        running_process_sem_id = semget(111 + running_PCB->proc->ID, 2, 0666);
        switching = 0;
    }

    // ── shut down MMU process ─────────────────────────────────
    mmu_shutdown();
    wait(NULL);  // wait for MMU child to exit

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
}

// ─────────────────────────────────────────────────────────────
//  MAIN
// ─────────────────────────────────────────────────────────────
int main(int argc, char *argv[])
{
    initClk();

    // create all message queues
    mmu_req_qid = msgget(MMU_REQ_KEY, 0666 | IPC_CREAT);
    mmu_res_qid = msgget(MMU_RES_KEY, 0666 | IPC_CREAT);
    mem_req_qid = msgget(MEM_REQ_KEY, 0666 | IPC_CREAT);
    mem_res_qid = msgget(MEM_RES_KEY, 0666 | IPC_CREAT);

    if (mmu_req_qid == -1 || mmu_res_qid == -1 ||
        mem_req_qid == -1 || mem_res_qid == -1) {
        perror("msgget failed");
        exit(1);
    }

    msgq_id = msgget(111, 0666);
    if (msgq_id == -1) { perror("msgget failed"); exit(1); }

    sem_id = semget(111, 1, 0666);
    if (sem_id == -1) { perror("semget failed"); exit(1); }

    // ── fork the MMU process ──────────────────────────────────
    pid_t mmu_pid = fork();
    if (mmu_pid == 0) {
        // child: become the MMU
        MMU_run(mmu_req_qid, mmu_res_qid);
        exit(0);  // never reached
    }

    // ── parent: run the scheduler ─────────────────────────────
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

    // cleanup message queues
    msgctl(mmu_req_qid, IPC_RMID, NULL);
    msgctl(mmu_res_qid, IPC_RMID, NULL);
    msgctl(mem_req_qid, IPC_RMID, NULL);
    msgctl(mem_res_qid, IPC_RMID, NULL);

    destroyClk(true);
    return 0;
}