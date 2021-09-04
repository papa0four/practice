#ifndef __MY_QUEUE_H__
#define __MY_QUEUE_H__

#define _XOPEN_SOURCE 700
#define _GNU_SOURCE

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#define CLEAN(a) if (a)free(a);(a)=NULL;
#define MAXQUEUE 50

/**
 * @brief - stores the file descriptor within the queue node
 * @member data - the actual file descriptor value
 */
typedef struct item {
    int data;
} item;

/**
 * @brief - the structure of the actual queue node
 * @member item - the node item storing the data
 * @member next - a pointer to the next node in the queue
 */
typedef struct node {
    item          item;
    struct node * next;
} node;

/**
 * @brief - the queue data structure itself
 * @member head - the pointer the front of the queue
 * @member tail - the pointer to the last item in the queue
 * @member queue_len - the current length of the queue
 */
typedef struct queue {
    node * head;
    node * tail;
    size_t queue_len;
} queue;

/**
 * QUEUE* INIT_QUEUE:
 * @brief - initialize everything required for the client socket fd queue
 * @param - N/A
 * @return - (queue *) pointer to queue structure, NULL on failure
 */
queue * init_queue();

/**
 * BOOL IS_FULL:
 * @brief - determine if current queue is at max capacity
 * @param fd_queue - pointer to current queue structure
 * @return - true if full, false otherwise
 */
bool is_full(const queue* fd_queue);

/**
 * BOOL IS_EMPTY:
 * @brief - determine if current queue is empty
 * @param fd_queue - pointer to current queue structure
 * @return: true if empty, false otherwise
 */
bool is_empty(const queue* fd_queue);

/**
 * SIZE_T GET_QUEUE_LEN:
 * @brief - get the current length of the client socket fd queue
 * @param fd_queue - pointer to current queue structure
 * @return - (size_t) current length of fd queue
 */
size_t get_queue_len(const queue* fd_queue);

/**
 * VOID TRAVERSE_QUEUE --- OPTIONAL, MEANT TO PRINT QUEUE NODE VALUES ---:
 * @brief - allows the user to traverse the current queue if need be
 * @param fd_queue - pointer to current queue structure
 * @param (* func_ptr)(item q_item) - function pointer with an item param
 * @return - N/A
 */
void traverse_queue(const queue* fd_queue, void (* func_ptr)(item q_item));

/**
 * VOID PEEK:
 * @brief - see what is in the head position of the queue
 * @param fd_queue - pointer to the current queue
 * @return - N/A
 */
void peek(queue* fd_queue);

/**
 * BOOL ENQUEUE:
 * @brief - place item into queue, head if empty, tail otherwise
 * @param fd_queue - pointer to current queue structure
 * @param q_item - item to be placed within the queue structure
 * @return - true upon success, false upon failure
 */
bool enqueue(queue* fd_queue, item q_item);

/**
 * INT DEQUEUE:
 * @brief - pop current queue head and return its item for use
 * @param fd_queue - pointer to current queue structure
 * @return - sockfd value upon success, -1 upon failure
 */
int dequeue(queue* fd_queue);

/**
 * VOID CLEAR:
 * @brief - properly cleans up queue resources
 * @param fd_queue - pointer to current queue structure
 * @return - N/A
 */
void clear(queue* fd_queue);

#endif
