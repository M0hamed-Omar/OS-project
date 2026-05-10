#include <math.h>
#include "MMU.h"

#define CONTEXT_SWITCH_TIME 1

int msgq_id;
int sem_id;
PCB *running_PCB;

void log_event(FILE *fp, int curr_time, char *state, PCB *p, int finished);

void RR_DEMO(int quantum, int k);
