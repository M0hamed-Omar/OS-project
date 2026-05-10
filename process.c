#include "headers.h"

int main(int argc, char *argv[])
{
    initClk();

    int remainingtime = atoi(argv[1]);
    int semid         = atoi(argv[2]);
    int process_id    = atoi(argv[3]);
    int mem_req_qid   = atoi(argv[4]);  // scheduler's mem request queue
    int mem_res_qid   = atoi(argv[5]);  // scheduler's mem response queue

    // load this process's request file
    char req_filename[64];
    sprintf(req_filename, "requests_%d.txt", process_id);
    FILE *req_file = fopen(req_filename, "r");

    typedef struct {
        int  cpu_tick;
        int  virtual_address;
        char rw;
    } MemRequest;

    MemRequest requests[256];
    int req_count = 0;

    if (req_file) {
        char line[128];
        while (fgets(line, sizeof(line), req_file)) {
            if (line[0] == '#' || line[0] == '\n') continue;
            char addr_bin[32], rw_str[4];
            int t;
            if (sscanf(line, "%d %s %s", &t, addr_bin, rw_str) == 3) {
                // convert binary string to integer
                int addr = 0;
                for (int i = 0; addr_bin[i]; i++)
                    addr = addr * 2 + (addr_bin[i] - '0');
                requests[req_count].cpu_tick        = t;
                requests[req_count].virtual_address = addr;
                requests[req_count].rw              = rw_str[0];
                req_count++;
            }
        }
        fclose(req_file);
    }

    // cpu_ticks_used tracks how many ticks this process has consumed.
    // It starts at 0 and is incremented every time the scheduler
    // releases semid[0] — including the 1-tick RAM access cost
    // (edit 1) and regular execution ticks.
    int cpu_ticks_used = 0;
    int req_index      = 0;

    int now;
    int prev_time = 0;
    
    while (1)
    {
        now = getClk();
        if (now == prev_time) continue;  // TICK GATE
        prev_time = now;
        
        // wait for scheduler to signal this tick
        printf("P: waiting\n");
        fflush(stdout);
        sem_wait(semid, 0);
        printf("P: awake\n");
    
        remainingtime--;
        int sent = 0;
        
        // check BEFORE incrementing: cpu_tick=0 fires on first CPU tick,
        // cpu_tick=1 fires on second, etc.
        while (req_index < req_count && requests[req_index].cpu_tick == cpu_ticks_used)
        {
            // send memory access request to scheduler
            struct mem_request req;
            req.mtype           = process_id;
            req.process_id      = process_id;
            req.virtual_address = requests[req_index].virtual_address;
            req.rw              = requests[req_index].rw;
            msgsnd(mem_req_qid, &req, sizeof(req) - sizeof(long), 0);
            sem_release(semid, 1);
            sent = 1;
            printf("P: sent and released\n");
            // block until scheduler resolves it.
            // if no fault: scheduler replies immediately (no extra tick).
            // if fault:    scheduler blocks us, handles disk (10 or 20
            //              ticks), then sends the reply — those disk ticks
            //              are NOT counted in cpu_ticks_used because the
            //              scheduler is not releasing semid[0] while we
            //              are blocked.
            struct mem_response res;
            msgrcv(mem_res_qid, &res, sizeof(res) - sizeof(long), process_id, 0);  // blocking wait
            
            req_index++;
        }

        cpu_ticks_used++;

        if(!sent) {
            sem_release(semid, 1);
            printf("P: released\n");
        }
        
        
        if (remainingtime == 0)
            break;
    }

    destroyClk(false);

    // notify scheduler we finished (SIGCHLD sent automatically on exit)
    sem_release(semid, 1);

    return 0;
}