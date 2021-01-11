#ifndef MY_QUEUE_H
#define MY_QUEUE_H

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

// define item struct
typedef struct item {
    // stores the client socket file descriptor
    int data;
} item; // end item struct definition

// define max allowable queue size
#define MAXQUEUE 50

// define node struct
typedef struct node {
    // a node can have an item to store data
    item item;
    // a pointer to the next node in the queue
    struct node* next;
} node; // end node struct definition

// define queue struct
typedef struct queue {
    // first node in queue
    node* head;
    // last node in queue
    node* tail;
    // current length of queue
    size_t queue_len;
} queue; // end queue struct definition

/**
 * QUEUE* INIT:
 *      PARAMETER: None
 *      BRIEF: initialize everything required for the client socket fd queue
 *      RETURN: (queue *) pointer to queue structure, NULL on failure
 */
queue* init();

/**
 * BOOL IS_FULL:
 *      PARAMETER: CONST QUEUE* FD_QUEUE - pointer to current queue structure
 *      BRIEF: determine if current queue is at max capacity
 *      RETURN: true if full, false otherwise
 */
bool is_full(const queue* fd_queue);

/**
 * BOOL IS_EMPTY:
 *      PARAMETER: CONST QUEUE* FD_QUEUE - pointer to current queue structure
 *      BRIEF: determine if current queue is empty
 *      RETURN: true if empty, false otherwise
 */
bool is_empty(const queue* fd_queue);

/**
 * SIZE_T GET_QUEUE_LEN:
 *      PARAMETER: CONST QUEUE* FD_QUEUE - pointer to current queue structure
 *      BRIEF: get the current length of the client socket fd queue
 *      RETURN: (size_t) current length of fd queue
 */
size_t get_queue_len(const queue* fd_queue);

/**
 * VOID TRAVERSE_QUEUE --- OPTIONAL, MEANT TO PRINT QUEUE NODE VALUES ---:
 *      PARAMETER: CONST QUEUE* FD_QUEUE - pointer to current queue structure
 *      PARAMETER: VOID (* FUNC_PTR)(ITEM Q_ITEM) - function pointer with an item param
 *      BRIEF: allows the user to traverse the current queue if need be
 *      RETURN: None
 */
void traverse_queue(const queue* fd_queue, void (* func_ptr)(item q_item));

/**
 * VOID PEEK:
 *      PARAMETER: QUEUE* FD_QUEUE - pointer to the current queue
 *      BRIEF: see what is in the head position of the queue
 *      RETURN: None
 */
void peek(queue* fd_queue);

/**
 * BOOL ENQUEUE:
 *      PARAMETER: QUEUE* FD_QUEUE - pointer to current queue structure
 *      PARAMETER: ITEM Q_ITEM - item to be placed within the queue structure
 *      BRIEF: place item into queue, head if empty, tail otherwise
 *      RETURN: true upon success, false upon failure
 */
bool enqueue(queue* fd_queue, item q_item);

/**
 * INT DEQUEUE:
 *      PARAMETER: QUEUE* FD_QUEUE - pointer to current queue structure
 *      BRIEF: pop current queue head and return its item for use
 *      RETURN: sockfd value upon success, -1 upon failure
 */
int dequeue(queue* fd_queue);

/**
 * VOID CLEAR:
 *      PARAMETER: QUEUE* FD_QUEUE - pointer to current queue structure
 *      BRIEF: properly cleans up queue resources
 *      RETURN: None
 */
void clear(queue* fd_queue);

#endif
