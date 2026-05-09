#include <stdio.h>
#include <stdlib.h>
#include "dqueue.h"

void initDQueue(DQueue *q)
{
    q->front = q->rear = NULL;
    q->size = 0;
}

int isDQueueEmpty(DQueue *q)
{
    return (q->size == 0);
}

void enqueueRear(DQueue *q, Process p)
{
    DQueueNode *newNode = (DQueueNode *)malloc(sizeof(DQueueNode));
    if (!newNode)
    {
        perror("Failed to allocate memory for queue node");
        exit(EXIT_FAILURE);
    }
    newNode->data = p;
    newNode->next = NULL;
    newNode->prev = q->rear;
    if (q->rear == NULL)
    {
        q->front = q->rear = newNode;
    }
    else
    {
        q->rear->next = newNode;
        q->rear = newNode;
    }
    q->size++;
}

void enqueueFront(DQueue *q, Process p)
{
    DQueueNode *newNode = (DQueueNode *)malloc(sizeof(DQueueNode));
    if (!newNode)
    {
        perror("Failed to allocate memory for queue node");
        exit(EXIT_FAILURE);
    }
    newNode->data = p;
    newNode->prev = NULL;
    newNode->next = q->front;
    if (q->front == NULL)
    {
        q->front = q->rear = newNode;
    }
    else
    {
        q->front->prev = newNode;
        q->front = newNode;
    }
    q->size++;
}

int dequeueFront(DQueue *q, Process *p)
{
    if (isDQueueEmpty(q))
    {
        return 0;
    }
    DQueueNode *temp = q->front;
    *p = temp->data;
    q->front = q->front->next;
    if (q->front == NULL)
    {
        q->rear = NULL;
    }
    else
    {
        q->front->prev = NULL;
    }
    free(temp);
    q->size--;
    return 1;
}

int dequeueRear(DQueue *q, Process *p)
{
    if (isDQueueEmpty(q))
    {
        return 0;
    }
    DQueueNode *temp = q->rear;
    *p = temp->data;
    q->rear = q->rear->prev;
    if (q->rear == NULL)
    {
        q->front = NULL;
    }
    else
    {
        q->rear->next = NULL;
    }
    free(temp);
    q->size--;
    return 1;
}

int peekDQueueFront(DQueue *q, Process *p)
{
    if (isDQueueEmpty(q))
    {
        return 0;
    }
    *p = q->front->data;
    return 1;
}

int peekDQueueRear(DQueue *q, Process *p)
{
    if (isDQueueEmpty(q))
    {
        return 0;
    }
    *p = q->rear->data;
    return 1;
}

void freeDQueue(DQueue *q)
{
    DQueueNode *current = q->front;
    while (current != NULL)
    {
        DQueueNode *temp = current;
        current = current->next;
        free(temp);
    }
    q->front = q->rear = NULL;
    q->size = 0;
}
