#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

void initQueue(Queue *q)
{
    q->front = q->rear = NULL;
    q->size = 0;
}

int isQueueEmpty(Queue *q)
{
    return (q->size == 0);
}

void enqueue(Queue *q, Process p)
{
    QueueNode *newNode = (QueueNode *)malloc(sizeof(QueueNode));
    if (!newNode)
    {
        perror("Failed to allocate memory for queue node");
        exit(EXIT_FAILURE);
    }
    newNode->data = p;
    newNode->next = NULL;
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

int dequeue(Queue *q, Process *p)
{
    if (isQueueEmpty(q))
    {
        return 0;
    }
    QueueNode *temp = q->front;
    *p = temp->data;
    q->front = q->front->next;
    if (q->front == NULL)
    {
        q->rear = NULL;
    }
    free(temp);
    q->size--;
    return 1;
}

int peekQueue(Queue *q, Process *p)
{
    if (isQueueEmpty(q))
    {
        return 0;
    }
    *p = q->front->data;
    return 1;
}

void freeQueue(Queue *q)
{
    QueueNode *current = q->front;
    while (current != NULL)
    {
        QueueNode *temp = current;
        current = current->next;
        free(temp);
    }
    q->front = q->rear = NULL;
    q->size = 0;
}
