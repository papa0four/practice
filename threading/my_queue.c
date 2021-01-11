#include "my_queue.h"

queue* init()
{
    queue* fd_queue = calloc(1, sizeof(queue));
    if (NULL == fd_queue)
    {
        errno = ENOMEM;
        perror("Could not allocate memory for queue in init");
        exit(-1);
    }
    fd_queue->head = fd_queue->tail = NULL;
    fd_queue->queue_len = 0;

    return fd_queue;
}

bool is_full(const queue* fd_queue)
{
    return fd_queue->queue_len == MAXQUEUE;
}

bool is_empty(const queue* fd_queue)
{
    return fd_queue->queue_len == 0;
}

size_t get_queue_len(const queue* fd_queue)
{
    return fd_queue->queue_len;
}

bool enqueue(queue* fd_queue, item q_item)
{
    if (is_full(fd_queue))
    {
        return false;
    }
    node* new_node = calloc(1, sizeof(node));
    if (NULL == new_node)
    {
        errno = ENOMEM;
        perror("Could not allocate memory for new_node");
        exit(-1);
    }
    new_node->item = q_item;
    new_node->next = NULL;
    if (is_empty(fd_queue))
    {
        fd_queue->head = new_node;
    }
    else
    {
        fd_queue->tail->next = new_node;
    }
    fd_queue->tail = new_node;
    fd_queue->queue_len++;
    return true;
}

void peek(queue* fd_queue)
{
    printf("Head of queue: %d\n", fd_queue->head->item.data);
}

int dequeue(queue* fd_queue)
{
    if (is_empty(fd_queue))
    {
        errno = EINVAL;
        perror("queue is empty");
        return -1;
    }
    item p_item = fd_queue->head->item;
    node* temp = fd_queue->head;
    fd_queue->head = fd_queue->head->next;
    free(temp);
    temp = NULL;
    fd_queue->queue_len--;
    if (0 == fd_queue->queue_len)
    {
        fd_queue->tail = NULL;
    }
    return p_item.data;
}

void traverse_queue(const queue* fd_queue, void (*func_ptr)(item q_item))
{
    node* cursor = fd_queue->head;
    while (cursor)
    {
        (*func_ptr)(cursor->item);
        cursor = cursor->next;
    }
}

void clear(queue* fd_queue)
{
    node* temp = NULL;
    while (false == is_empty(fd_queue))
    {
        temp = fd_queue->head;
        dequeue(fd_queue);
        fd_queue->head = fd_queue->head->next;
        free(temp);
        temp = NULL;
    }
}