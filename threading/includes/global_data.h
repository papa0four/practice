#ifndef __GLOBAL_DATA_H__
#define __GLOBAL_DATA_H__

#define _XOPEN_SOURCE 700
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include "server.h"
#include "my_queue.h"

#define SERV_ADDR       "127.0.0.1"
#define MAX_CLIENTS     50

/**
 * @brief - the structure storing the threadpool data for client connections
 * @member threads - an array of pthread_t types for each connection made
 * @member condition - the condition variable used for locks to set as wait or 
 *                     broadcast
 * @member lock - the pthread_mutex_lock variable to lock critical sections during
 *                execution
 * @member max_thread_cnt - the total number of allowable client connections
 *                          (set at server program execution from the cmdline)
 * @member thread_cnt - the current number of active threads
 */
typedef struct thread_pool
{
    pthread_t       * threads;
    pthread_cond_t    condition;
    pthread_mutex_t   lock;
    size_t            max_thread_cnt;
    size_t            thread_cnt;
} threadpool_t;

/**
 * @brief - a list of global variables used throughout the lifespan of the program
 * @var serv_running - a global boolean flag to determine life of server
 * @var exiting - a thread specific boolean flag to determine if client is connected
 * @var clients_con - a count of all active connections
 * @var p_tpool - a pointer to the threadpool structure
 * @var p_queue - a pointer to the queue structure
 */
extern bool            serv_running;
extern bool            exiting;
extern size_t          clients_con;
extern threadpool_t  * p_tpool;
extern queue         * p_queue;

/**
 * @brief - initializes all global variables to include the threadpool and queue
 * @param - N/A
 * @return - 0 on success, -1 on failure
 */
int init_globals ();

/**
 * INT INIT_THREADPOOL:
 * @brief initializes threadpool holding specified number of threads setting
 *        them to an idle state waiting for handoff to the client
 * @param p_tpool - pointer to the pool struct
 * @param num_of_clients - number of current connections
 * @return - (int) 0 on success, -1 on error
 */
int init_threadpool (threadpool_t * p_tpool, size_t num_of_clients);

/**
 * VOID POOL_CLEANUP:
 * @brief - joins all active threads and cleans up resources for proper clean up
 * @param num_threads - number of actual threads created and connected
 * @return - N/A
 */
void pool_cleanup(size_t num_threads);

#endif