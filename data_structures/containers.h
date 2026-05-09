#ifndef CONTAINERS_H
#define CONTAINERS_H

typedef struct Process
{
    int ID;
    int priority;
    int arrivalTime;
    int runTime;
    int PID;
    int base;
    int limit;
} Process;

typedef enum State
{
    READY,
    RUNNING,
    BLOCKED
} State;

typedef struct PCB
{
    Process *proc;
    State state;
    int remainingTime;
    int waitingTime;
    int last_start_time;
    int page_table_frame;   // which physical frame holds this process's page table
    int page_table_index;   // which physical frame holds this process's page table
    int base;               // starting page number on disk
    int limit;              // number of virtual pages
    int cpu_ticks_used;     // tracks when to trigger memory requests
    int pending_frame;
    int disk_done_at;
    int pending_vpn;
    int fault_rw;
    int disk_ticks;
} PCB;

#endif 