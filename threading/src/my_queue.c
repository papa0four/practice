#include "../includes/my_queue.h"

queue * init_queue()
{
    queue * fd_queue = calloc(1, sizeof(queue));
    if (NULL == fd_queue)
    {
        errno = ENOMEM;
        perror("Could not allocate memory for queue in init");
        return NULL;
    }

    fd_queue->head = fd_queue->tail = NULL;
    fd_queue->queue_len = 0;

    return fd_queue;
}

bool is_full (const queue * fd_queue)
{
    return fd_queue->queue_len == MAXQUEUE;
}

bool is_empty(const queue* fd_queue)
{
    return fd_queue->queue_len == 0;
}

size_t get_queue_len(const queue * fd_queue)
{
    return fd_queue->queue_len;
}

bool enqueue(queue * fd_queue, item q_item)
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
        fd_queue->head = fd_queue->tail = new_node;
        fd_queue->queue_len++;
        return true;
    }
    
    node * p_current = fd_queue->head;
    while (p_current)
    {
        p_current = p_current->next;
    }

    p_current->next = new_node;
    fd_queue->tail  = new_node;
    fd_queue->queue_len++;

    return true;    
}

void peek(queue * fd_queue)
{
    printf("Head of queue: %d\n", fd_queue->head->item.data);
}

int dequeue(queue * fd_queue)
{
    if (is_empty(fd_queue))
    {
        errno = EINVAL;
        perror("queue is empty");
        return -1;
    }

    item   p_item = fd_queue->head->item;
    node * p_temp = fd_queue->head;

    fd_queue->head = fd_queue->head->next;

    CLEAN(p_temp);
    fd_queue->queue_len--;

    if (true == is_empty(fd_queue))
    {
        fd_queue->head = fd_queue->tail = NULL;
    }

    return p_item.data;
}

void traverse_queue(const queue * fd_queue, void (*func_ptr)(item q_item))
{
    if ((NULL == fd_queue) || (NULL == fd_queue->head) || NULL == (func_ptr))
    {
        errno = EINVAL;
        fprintf(stderr, "%s one or more parameters passed are NULL: %s\n", __func__, strerror(errno));
        return;
    }

    node * cursor = fd_queue->head;

    while (cursor)
    {
        (*func_ptr)(cursor->item);
        cursor = cursor->next;
    }
}

void clear(queue * fd_queue)
{
    if (NULL == fd_queue)
    {
        printf("Queue pointer passed is NULL\n");
        return;
    }

    node * p_temp = NULL;

    while (false == is_empty(fd_queue))
    {
        if (fd_queue->head == fd_queue->tail)
        {
            p_temp = fd_queue->head;
            CLEAN(p_temp);
            fd_queue->head = NULL;
            fd_queue->tail = NULL;
            return;
        }
        p_temp = fd_queue->head;
        fd_queue->head = fd_queue->head->next;
        dequeue(fd_queue);
        CLEAN(p_temp);
    }
}