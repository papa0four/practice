#include "../includes/global_data.h"

bool            serv_running;
bool            exiting;
size_t          clients_con;
threadpool_t  * p_tpool;
queue         * p_queue;

int init_globals ()
{
    serv_running = true;
    exiting      = false;
    clients_con  = 0;
    
    p_tpool = calloc(1, sizeof(threadpool_t));
    if (NULL == p_tpool)
    {
        errno = ENOMEM;
        fprintf(stderr, "%s could not allocate space for threadpool: %s\n", __func__, strerror(errno));
        return -1;
    }

    p_queue = init_queue();
    if (NULL == p_queue)
    {
        fprintf(stderr, "%s error occurred initializing queue data structure\n", __func__);
        CLEAN(p_tpool);
        return -1;
    }

    return 0;
}

int init_threadpool(threadpool_t * p_tpool, size_t num_of_clients)
{
    if (NULL == p_tpool)
    {
        errno = EINVAL;
        perror("Pool passed is NULL");
        return -1;
    }

    if (0 == num_of_clients)
    {
        num_of_clients = MAX_CLIENTS;
    }

    p_tpool->threads = calloc(num_of_clients, sizeof(pthread_t));
    if (NULL == p_tpool->threads)
    {
        errno = ENOMEM;
        perror("Could not allocate memory for p_tpool->threads");
        free(p_tpool);
        p_tpool = NULL;
        return -1;
    }

    p_tpool->max_thread_cnt = num_of_clients;
    pthread_mutex_init(&p_tpool->lock, NULL);
    pthread_cond_init(&p_tpool->condition, NULL);

    for (size_t idx = 0; idx < num_of_clients; idx++)
    {
        if (0 != pthread_create(&p_tpool->threads[idx], NULL, server_func, NULL))
        {
            errno = EINVAL;
            perror("Error creating thread");
        }
    }

    return 0;
}

void pool_cleanup (size_t num_threads)
{
    if ((NULL == p_tpool) || (NULL == p_tpool->threads))
    {
        fprintf(stderr, "%s pointer to the threadpool is NULL\n", __func__);
        return;
    }
    pthread_cond_broadcast(&p_tpool->condition);
    for (size_t idx = 0; idx < num_threads; idx++)
    {
        if (p_tpool->threads[idx])
        {
            pthread_join(p_tpool->threads[idx], NULL);
        }
    }

    clear(p_queue);
    CLEAN(p_tpool->threads);
    pthread_mutex_destroy(&p_tpool->lock);
    pthread_cond_destroy(&p_tpool->condition);
    CLEAN(p_tpool);
}