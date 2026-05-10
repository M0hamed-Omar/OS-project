#include <stdio.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "data_structures/queue.h"
#include "data_structures/dqueue.h"
#include "data_structures/Priority_queue.h"
#include "data_structures/pcb_queue.h"
#include "data_structures/pcb2_deque.h"
#include "data_structures/containers.h"

#define true 1
#define false 0

#define SHKEY 300

// message to send memory request to scheduler
struct mem_request {
    long mtype;
    int process_id;
    int virtual_address;
    char rw;         // 'r' or 'w'
};

// message to receive page fault response from scheduler
struct mem_response {
    long mtype;
    int granted;     // 1 = page is ready, process can continue
};

typedef short bool;
typedef struct msbuf
{
    long mtype;
    Process proc;
};

int getClk();


/*
 * All process call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
 */
void initClk();

/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
 */

void destroyClk(bool terminateAll);


union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};


int create_semaphore(key_t key, int initial_value);

int sem_wait(int semid, int index);

int sem_release(int semid, int index);

int process_scheduler_sem(int id);