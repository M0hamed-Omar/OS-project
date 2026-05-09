#include "headers.h"

int *shmaddr; 

int getClk()
{
    return *shmaddr;
}

void initClk()
{
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *)shmat(shmid, (void *)0, 0);
}

void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}

int create_semaphore(key_t key, int initial_value) {
    int semid = semget(key, 1, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget");
        return -1;
    }

    union semun arg;
    arg.val = initial_value;

    if (semctl(semid, 0, SETVAL, arg) == -1) {
        perror("semctl");
        return -1;
    }

    return semid;
}

int sem_wait(int semid, int index) {
    struct sembuf op;

    op.sem_num = index;  
    op.sem_op  = -1;     
    op.sem_flg = 0;

    return semop(semid, &op, 1); 
}

int sem_release(int semid, int index) {
    struct sembuf op;

    op.sem_num = index;
    op.sem_op  = +1;
    op.sem_flg = 0;

    return semop(semid, &op, 1);
}

int process_scheduler_sem(int id) {
    
    int semid = semget(111 + id, 2, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget");
        return -1;
    }
    union semun arg;
    unsigned short values[2];

    values[0] = 0; 
    values[1] = 0; 

    arg.array = values;

    if (semctl(semid, 0, SETALL, arg) == -1) {
        perror("semctl SETALL");
        return -1;
    }

    return semid;
}