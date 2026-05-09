#ifndef DQUEUE_H
#define DQUEUE_H
#include "containers.h"

typedef struct DQueueNode
{
    Process data;
    struct DQueueNode *next;
    struct DQueueNode *prev;
} DQueueNode;

typedef struct DQueue
{
    DQueueNode *front;
    DQueueNode *rear;
    int size;
} DQueue;

void initDQueue(DQueue *q);
int isDQueueEmpty(DQueue *q);
void enqueueRear(DQueue *q, Process p);
void enqueueFront(DQueue *q, Process p);
int dequeueFront(DQueue *q, Process *p);
int dequeueRear(DQueue *q, Process *p);
int peekDQueueFront(DQueue *q, Process *p);
int peekDQueueRear(DQueue *q, Process *p);
void freeDQueue(DQueue *q);

#endif
