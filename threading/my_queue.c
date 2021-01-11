#include "my_queue.h"

/**
 * QUEUE* INIT:
 *      PARAMETER: None
 *      BRIEF: initialize everything required for the client socket fd queue
 *      RETURN: (queue *) pointer to queue structure, NULL on failure
 */
queue* init()
{
    // allocate memory for the queue
    queue* fd_queue = calloc(1, sizeof(queue));
    // check for memory allocation failure
    if (NULL == fd_queue)
    {
        errno = ENOMEM;
        perror("Could not allocate memory for queue in init");
        return NULL;
    }
    // set the head and tail to NULL
    fd_queue->head = fd_queue->tail = NULL;
    // set current length to 0
    fd_queue->queue_len = 0;
    // return pointer to created queue structure
    return fd_queue;
}

/**
 * BOOL IS_FULL:
 *      PARAMETER: CONST QUEUE* FD_QUEUE - pointer to current queue structure
 *      BRIEF: determine if current queue is at max capacity
 *      RETURN: true if full, false otherwise
 */
bool is_full(const queue* fd_queue)
{
    // check if current queue length is equal to MAX
    return fd_queue->queue_len == MAXQUEUE;
}

/**
 * BOOL IS_EMPTY:
 *      PARAMETER: CONST QUEUE* FD_QUEUE - pointer to current queue structure
 *      BRIEF: determine if current queue is empty
 *      RETURN: true if empty, false otherwise
 */
bool is_empty(const queue* fd_queue)
{
    // check if current queue length is 0
    return fd_queue->queue_len == 0;
}

/**
 * SIZE_T GET_QUEUE_LEN:
 *      PARAMETER: CONST QUEUE* FD_QUEUE - pointer to current queue structure
 *      BRIEF: get the current length of the client socket fd queue
 *      RETURN: (size_t) current length of fd queue
 */
size_t get_queue_len(const queue* fd_queue)
{
    // return current queue length variable
    return fd_queue->queue_len;
}

/**
 * BOOL ENQUEUE:
 *      PARAMETER: QUEUE* FD_QUEUE - pointer to current queue structure
 *      PARAMETER: ITEM Q_ITEM - item to be placed within the queue structure
 *      BRIEF: place item into queue, head if empty, tail otherwise
 *      RETURN: true upon success, false upon failure
 */
bool enqueue(queue* fd_queue, item q_item)
{
    // check if queue passed is full
    if (is_full(fd_queue))
    {
        return false;
    }
    // allocate memory for a new node structure
    node* new_node = calloc(1, sizeof(node));
    // check for memory allocation failure
    if (NULL == new_node)
    {
        errno = ENOMEM;
        perror("Could not allocate memory for new_node");
        exit(-1);
    }
    // set node item member to passed q_item
    new_node->item = q_item;
    // set next pointer to NULL
    new_node->next = NULL;
    // if current queue is empty
    if (is_empty(fd_queue))
    {
        // set new node as queue head
        fd_queue->head = new_node;
    }
    // if current queue is not empty
    else
    {
        // set current tail's next pointer to the new node
        fd_queue->tail->next = new_node;
    }
    // make the new node the new queue tail
    fd_queue->tail = new_node;
    // increment queue length by 1
    fd_queue->queue_len++;
    // return true on success
    return true;
}

/**
 * VOID PEEK:
 *      PARAMETER: QUEUE* FD_QUEUE - pointer to the current queue
 *      BRIEF: see what is in the head position of the queue
 *      RETURN: None
 */
void peek(queue* fd_queue)
{
    // print the value at the head of the queue
    printf("Head of queue: %d\n", fd_queue->head->item.data);
}

/**
 * INT DEQUEUE:
 *      PARAMETER: QUEUE* FD_QUEUE - pointer to current queue structure
 *      BRIEF: pop current queue head and return its item for use
 *      RETURN: sockfd value upon success, -1 upon failure
 */
int dequeue(queue* fd_queue)
{
    // check if passed queue is empty, print error message and return -1
    if (is_empty(fd_queue))
    {
        errno = EINVAL;
        perror("queue is empty");
        return -1;
    }
    // create an item variable and set to current queue head item
    item p_item = fd_queue->head->item;
    // create a temp node pointer to current queue head
    node* temp = fd_queue->head;
    // set queue head pointer to the next node in the queue
    fd_queue->head = fd_queue->head->next;
    // free the current head node
    free(temp);
    temp = NULL;
    // decrement the current queue length by 1
    fd_queue->queue_len--;
    // if no nodes are in the queue
    if (0 == fd_queue->queue_len)
    {
        // nullify the head and tail pointers
        fd_queue->head = fd_queue->tail = NULL;
    }
    // return the data in the current queue head on success
    return p_item.data;
}

/**
 * VOID TRAVERSE_QUEUE --- OPTIONAL, MEANT TO PRINT QUEUE NODE VALUES ---:
 *      PARAMETER: CONST QUEUE* FD_QUEUE - pointer to current queue structure
 *      PARAMETER: VOID (* FUNC_PTR)(ITEM Q_ITEM) - function pointer with an item param
 *      BRIEF: allows the user to traverse the current queue if need be
 *      RETURN: None
 */
void traverse_queue(const queue* fd_queue, void (*func_ptr)(item q_item))
{
    // set current node to head
    node* cursor = fd_queue->head;
    // iterate over queue nodes
    while (cursor)
    {
        // meant to call print value function on each item
        (*func_ptr)(cursor->item);
        // point to the next node in the queue
        cursor = cursor->next;
    }
}

/**
 * VOID CLEAR:
 *      PARAMETER: QUEUE* FD_QUEUE - pointer to current queue structure
 *      BRIEF: properly cleans up queue resources
 *      RETURN: None
 */
void clear(queue* fd_queue)
{
    // check if passed queue ptr is null
    if (NULL == fd_queue)
    {
        printf("Queue pointer passed is NULL\n");
        return;
    }
    // create a temp node pointer
    node* temp = NULL;
    // iterate over the queue while nodes exist
    while (false == is_empty(fd_queue))
    {
        //set temp node equal to current queue head
        temp = fd_queue->head;
        // call dequeue on the current node
        dequeue(fd_queue);
        // set current head pointer to the next queue node
        fd_queue->head = fd_queue->head->next;
        // free the temp node and nullify its value
        free(temp);
        temp = NULL;
    }
}