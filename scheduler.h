#include <math.h>
#include "MMU.h"

#define CONTEXT_SWITCH_TIME 1
#define RAM_ACCESS_TIME 1
#define SHM_KEY_FOR_2CPU 222
#define SEM_KEY_FOR_2CPU 333
#define CPU_COUNT 2
#define STEAL_OVERHEAD 3
#define MANAGED 0
#define MANAGER 1
#define MANAGER2 2
#define MANAGED2 3

typedef struct {
    int total_remaining;
    int q_size;
    int finished;
    PCB2 last_PCB;
} Scheduler_Info;

typedef struct {
    Scheduler_Info S[2];
    PCB2 new_pcb;
    int turn;
    int done;
} shared_data;

int msgq_id;
int sem_id;
PCB *running_PCB;

void log_event(FILE *fp,int curr_time, char *state, PCB *p, int finished);
void log_event_2CPU(FILE *fp,int curr_time, PCB2 *pcb, int status);

void RR_DEMO(int quantum, int k);
// void HPF_DEMO();
// void MULTI_CPU_DEMO(int N, int M);
// void cpu_worker(int cpu_id, int Schedulers_shm_id);
// void clear_2CPU_resources(int signum);
// void clear_2CPU_resources_managed(int signum);
