#include <stdio.h>
#include <stdlib.h>
#include "Priority_queue.h"

void enqueuePri(PriNode **head, PCB *P)
{
    PriNode *newNode = (PriNode *)malloc(sizeof(PriNode));
    newNode->data = P;
    newNode->next = NULL;

    if (*head == NULL || P->proc->priority < (*head)->data->proc->priority)
    {
        newNode->next = *head;
        *head = newNode;
        return;
    }

    PriNode *temp = *head;
    while (temp->next != NULL &&
           temp->next->data->proc->priority <= P->proc->priority)
    {
        temp = temp->next;
    }

    newNode->next = temp->next;
    temp->next = newNode;
}

int dequeuePri(PriNode **head)
{
    if (*head == NULL)
        return -1;

    PriNode *temp = *head;
    *head = (*head)->next;

    free(temp);
    return 1;
}

int isEmptyPri(PriNode *head)
{
    return head == NULL;
}

PCB *peekPri(PriNode *head)
{
    if(head == NULL)
        return NULL;
    return head->data;
}

void initPriQueue(PriNode **head){
    *head = NULL;
}

void printPriQueue(PriNode *head)
{
    if (head == NULL)
    {
        printf("Priority Queue is empty\n");
        return;
    }

    PriNode *temp = head;

    printf("Priority Queue:\n");

    while (temp != NULL)
    {
        if (temp->data != NULL && temp->data->proc != NULL)
        {
            printf("Process ID: %d | Priority: %d\n",
                   temp->data->proc->ID,
                   temp->data->proc->priority);
        }
        else
        {
            printf("NULL node encountered\n");
        }

        temp = temp->next;
    }

    printf("-------------------------\n");
}