#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int sem_id;
    int remaining_time;
    int arrival_time;
    int total_time;
    int ID;
} PCB2;

typedef struct {
    PCB2 *data;
    struct Node *next;
    struct Node *prev;
} DeqNode;

typedef struct {
    DeqNode *head;
    DeqNode *tail;
    int size;
} PCB2_Deque;

void initDeque(PCB2_Deque *dq);

int isEmpty(PCB2_Deque *dq);

int getSize(PCB2_Deque *dq);

void pushFront(PCB2_Deque *dq, PCB2 *value);

void pushBack(PCB2_Deque *dq, PCB2 *value);

PCB2 *popFront(PCB2_Deque *dq);

PCB2 *popBack(PCB2_Deque *dq);

PCB2 *peekFront(PCB2_Deque *dq);

PCB2 *peekBack(PCB2_Deque *dq);

void clearDeque(PCB2_Deque *dq);

void printDeque(PCB2_Deque *dq);