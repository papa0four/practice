#ifndef __SERVER_H__
#define __SERVER_H__

#define _XOPEN_SOURCE 700
#define _GNU_SOURCE

#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <dirent.h>

#include "my_queue.h"
#include "file_operations.h"

#define SERV_ADDR       "127.0.0.1"
#define MAX_CLIENTS     50
#define LIST            100
#define DOWNLOAD        200
#define UPLOAD          300
#define EXIT            500

/**
 * 
 */
typedef struct thread_pool
{
    pthread_t* threads;
    size_t max_thread_cnt;
    size_t thread_cnt;
    pthread_mutex_t lock;
    pthread_cond_t condition;
} threadpool;

/**
 * VOID HANDLE_SIGNAL:
 * @brief - handle ctrl+c input from user
 * @param sig_no: integer representation for SIGINT
 * @return - N/A
 */
void handle_signal (int sig_no);

/**
 * VOID* SERVER_FUNC:
 * @brief - handles the server functionality to be passed to each thread
 * @param - N/A
 * @return - (void *) NULL on success, err_code 999 on failure
 */
void * server_func ();

/**
 * VOID END_CONNECTION:
 * @brief - handles the graceful termination of a client connection
 * @param - N/A
 * @return - N/A
 */
void end_connection ();

/**
 * BOOL IP_IS_VALID - OPTIONAL:
 * @brief - meant to determine if the ip to be used is valid
 * @param p_ipaddr - ip address passed to be used by server for network
 *                                  connection
 * @return - (bool) true/false value whether ip address is valid
 */
bool ip_is_valid (char * p_ipaddr);

/**
 * INT INIT_THREADPOOL:
 * @brief initializes threadpool holding specified number of threads setting
 *        them to an idle state waiting for handoff to the client
 * @param p_tpool - pointer to the pool struct
 * @param num_of_clients - number of current connections
 * @return - (int) 0 on success, -1 on error
 */
int init_threadpool (threadpool* p_tpool, size_t num_of_clients);

/**
 * VOID HANDLE_INCOMING_CLIENT:
 * @brief - obtain socket fd from queue and hand it off to thread for
 *          for network connection to server
 * @param client_fd - client socket file descriptor
 * @return - N/A
 */
void handle_incoming_client (int client_fd);

/**
 * VOID POOL_CLEANUP:
 * @brief - joins all active threads and cleans up resources for proper clean up
 * @param num_threads - number of actual threads created and connected
 * @return - N/A
 */
void pool_cleanup(size_t num_threads);

#endif
