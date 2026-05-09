#ifndef PCB_QUEUE_H
#define PCB_QUEUE_H

#include "containers.h"

typedef struct PCBNode
{
    PCB *data;
    struct PCBNode *next;
} PCBNode;

// Function declarations
void initPcbQueue(PCBNode **head);
void enqueuePcb(PCBNode **head, PCB *P);
int dequeuePcb(PCBNode **head);
int isEmptyPcb(PCBNode *head);
PCB *peekPcb(PCBNode *head);
void printPcbQueue(PCBNode *head);

#endif