#ifndef MY_QUEUE_H
#define MY_QUEUE_H

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

typedef struct item {
    int data;
} item;

#define MAXQUEUE 100

typedef struct node {
    item item;
    struct node* next;
} node;

typedef struct queue {
    node* head;
    node* tail;
    size_t queue_len;
} queue;

queue* init();
bool is_full(const queue* fd_queue);
bool is_empty(const queue* fd_queue);
size_t get_queue_len(const queue* fd_queue);
void traverse_queue(const queue* fd_queue, void (* func_ptr)(item q_item));
void peek(queue* fd_queue);
bool enqueue(queue* fd_queue, item q_item);
int dequeue(queue* fd_queue);
void clear(queue* fd_queue);

#endif
