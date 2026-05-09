#ifndef QUEUE_H
#define QUEUE_H
#include "containers.h"

typedef struct QueueNode
{
    Process data;
    struct QueueNode *next;
} QueueNode;

typedef struct Queue
{
    QueueNode *front;
    QueueNode *rear;
    int size;
} Queue;

void initQueue(Queue *q);
int isQueueEmpty(Queue *q);
void enqueue(Queue *q, Process p);
int dequeue(Queue *q, Process *p);
int peekQueue(Queue *q, Process *p);
void freeQueue(Queue *q);

#endif
