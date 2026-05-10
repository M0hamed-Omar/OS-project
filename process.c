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

    int cpu_ticks_used       = 0;
    int req_index            = 0;
    int waiting_for_response = 0; // set after sending a request; cleared next tick
    int normal   = 0;
    int prev_time = -1;
    int clk_now = -1;

    while (1)
    {
        // wait for scheduler's tick signal
        printf("P%d: waiting\n", process_id);
        sem_wait(semid, 0);
        printf("P%d: awake\n", process_id);
        
        // if(normal && getClk() - prev_time > 1) {
        //     cpu_ticks_used--;
        //     remainingtime++;
        // }

        normal = 0;

        int clk_now = getClk();
        prev_time = clk_now;

        printf("[P%d | clk=%d | cpu_tick=%d | remaining=%d] tick\n",
               process_id, clk_now, cpu_ticks_used, remainingtime);
        fflush(stdout);

        // ── STEP 1: receive pending response from last tick ──────
        if (waiting_for_response) {
            printf("[P%d | clk=%d] waiting for mem_response...\n",
                   process_id, clk_now);
            fflush(stdout);

            struct mem_response res;
            msgrcv(mem_res_qid, &res, sizeof(res) - sizeof(long), process_id, 0);

            printf("[P%d | clk=%d] <<< response received (granted=%d)\n",
                   process_id, getClk(), res.granted);
            fflush(stdout);

            waiting_for_response = 0;
            req_index++;
        }

        // ── STEP 2: send new request if one is due this cpu_tick ─
        int sent = 0;

        if (req_index < req_count && requests[req_index].cpu_tick == cpu_ticks_used) {
            struct mem_request req;
            req.mtype           = process_id;
            req.process_id      = process_id;
            req.virtual_address = requests[req_index].virtual_address;
            req.rw              = requests[req_index].rw;

            printf("[P%d | clk=%d | cpu_tick=%d] >>> sending mem_request VA=0x%x rw=%c\n",
                   process_id, clk_now, cpu_ticks_used,
                   req.virtual_address, req.rw);
            fflush(stdout);

            msgsnd(mem_req_qid, &req, sizeof(req) - sizeof(long), 0);
            waiting_for_response = 1;
            sent = 1;
        }

        if (!sent) {
            printf("[P%d | clk=%d | cpu_tick=%d] normal execution\n",
                   process_id, clk_now, cpu_ticks_used);
            normal = 1;
            fflush(stdout);
        }

        if (sent) {
            // Only real CPU work ticks decrement remaining time.
            // A RAM-request send tick is overhead — don't count it.
            // remainingtime--;
            // cpu_ticks_used++;
        }
        
        remainingtime--;
        cpu_ticks_used++;

        // always signal the scheduler that this tick is done
        sem_release(semid, 1);

        if (remainingtime == 0)
            break;
    }

    destroyClk(false);

    // notify scheduler we finished
    sem_release(semid, 1);

    return 0;
}