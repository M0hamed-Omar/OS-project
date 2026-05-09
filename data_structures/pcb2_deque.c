#include "pcb2_deque.h"

void initDeque(PCB2_Deque *dq) {
    dq->head = dq->tail = NULL;
    dq->size = 0;
}

int isEmpty(PCB2_Deque *dq) {
    return dq->size == 0;
}

int getSize(PCB2_Deque *dq) {
    return dq->size;
}

void pushFront(PCB2_Deque *dq, PCB2 *value) {
    DeqNode *newNode = (DeqNode *)malloc(sizeof(DeqNode));
    newNode->data = value;
    newNode->prev = NULL;
    newNode->next = dq->head;

    if (isEmpty(dq)) {
        dq->head = dq->tail = newNode;
    } else {
        dq->head->prev = newNode;
        dq->head = newNode;
    }

    dq->size++;
}

void pushBack(PCB2_Deque *dq, PCB2 *value) {
    DeqNode *newNode = (DeqNode *)malloc(sizeof(DeqNode));
    newNode->data = value;
    newNode->next = NULL;
    newNode->prev = dq->tail;

    if (isEmpty(dq)) {
        dq->head = dq->tail = newNode;
    } else {
        dq->tail->next = newNode;
        dq->tail = newNode;
    }

    dq->size++;
}

PCB2 *popFront(PCB2_Deque *dq) {
    if (isEmpty(dq)) {
        return NULL;
    }

    DeqNode *temp = dq->head;
    PCB2 *val = temp->data;

    dq->head = dq->head->next;
    if (dq->head != NULL)
        dq->head->prev = NULL;
    else
        dq->tail = NULL;

    free(temp);
    dq->size--;

    return val;
}

PCB2 *popBack(PCB2_Deque *dq) {
    if (isEmpty(dq)) {
        return NULL;
    }

    DeqNode *temp = dq->tail;
    PCB2 *val = temp->data;

    dq->tail = dq->tail->prev;
    if (dq->tail != NULL)
        dq->tail->next = NULL;
    else
        dq->head = NULL;

    free(temp);
    dq->size--;

    return val;
}

PCB2 *peekFront(PCB2_Deque *dq) {
    if (isEmpty(dq)) {
        return NULL;
    }
    return dq->head->data;
}

PCB2 *peekBack(PCB2_Deque *dq) {
    if (isEmpty(dq)) {
        return NULL;
    }
    return dq->tail->data;
}

void clearDeque(PCB2_Deque *dq) {
    while (!isEmpty(dq)) {
        popFront(dq);
    }
}

void printDeque(PCB2_Deque *dq) {
    DeqNode *curr = dq->head;
    while (curr) {
        printf("[sem=%d, rem=%d] ",
               curr->data->sem_id,
               curr->data->remaining_time);
        curr = curr->next;
    }
    printf("\n");
}