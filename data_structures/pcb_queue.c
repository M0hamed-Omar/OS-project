#include "pcb_queue.h"
#include <stdlib.h>
#include <stdio.h>

void initPcbQueue(PCBNode **head)
{
    *head = NULL;
}

int isEmptyPcb(PCBNode *head)
{
    return head == NULL;
}

void enqueuePcb(PCBNode **head, PCB *P)
{
    PCBNode *node = (PCBNode *)malloc(sizeof(PCBNode));
    node->data = P;
    node->next = NULL;
    if (*head == NULL)
    {
        *head = node;
    }
    else
    {
        PCBNode *curr = *head;
        while (curr->next != NULL)
            curr = curr->next;
        curr->next = node;
    }
}

int dequeuePcb(PCBNode **head)
{
    if (isEmptyPcb(*head))
        return 0;
    PCBNode *node = *head;
    *head = node->next;
    free(node);
    return 1;
}

PCB *peekPcb(PCBNode *head)
{
    if (isEmptyPcb(head))
        return NULL;
    return head->data;
}

void printPcbQueue(PCBNode *head)
{
    PCBNode *curr = head;
    printf("PCB Queue: ");
    while (curr != NULL)
    {
        printf("%d ", (void *)curr->data->proc->ID);
        curr = curr->next;
    }
    printf("\n");
}