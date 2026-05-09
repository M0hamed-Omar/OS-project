#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include "containers.h"

typedef struct PriNode
{
    PCB *data;
    struct PriNode *next;
} PriNode;

void initPriQueue(PriNode **head);
void enqueuePri(PriNode **head, PCB *P);
int dequeuePri(PriNode **head);
int isEmptyPri(PriNode *head);
PCB *peekPri(PriNode *head);
void printPriQueue(PriNode *head);

#endif