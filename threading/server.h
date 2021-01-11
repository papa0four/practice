#ifndef SERVER_H
#define SERVER_H

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

// declare thread pool structure
typedef struct _thread_pool threadpool;
// declare dirent structure for directory traversal
struct dirent* dir;

// define thread pool structure
typedef struct _thread_pool {
    // array of threads, pthread_t type
    pthread_t* threads;
    // max allowable threads to be created
    size_t max_thread_cnt;
    // current number of threads alive
    size_t thread_cnt;
    // mutex lock
    pthread_mutex_t lock;
    // thread signal condition
    pthread_cond_t condition;
}threadpool; // end structure defintion

/**
 * VOID HANDLE_SIGNAL:
 *      PARAMETER: INT SIG_NO - SIGINT value
 *      BRIEF: handle ctrl+c input from user
 *      RETURN: None
 */
void handle_signal(int sig_no);

/**
 * INT GET_FILE_COUNT:
 *      PARAMETER: INT SOCKFD - client socket file descriptor
 *      BRIEF: determines the number of files located within the file server
 *      RETURN: (int) number of DT_REG files in FileServer directory, -1 on error
 */
int get_file_count(int sockfd);

/**
 * VOID LIST_DIR:
 *      PARAMETER: INT SOCKFD - client socket file descriptor
 *      BRIEF: creates a list of DT_REG files to send to client (similar to basic ls cmd)
 *      RETURN: None
 */
void list_dir(int sockfd);

/**
 * BOOL IS_FILE:
 *      PARAMETER: CHAR* FILE_NAME - name of file for validity check
 *      BRIEF: determine whether passed file exists within the directory
 *      RETURN: (bool) true/false value whether file exists
 */
bool is_file(char* file_name);

/**
 * VOID DOWNLOAD_FILE:
 *      PARAMETER: CHAR* FILE_PASSED - file within server to be sent to client
 *      PARAMETER: INT SOCKFD - client socket file descriptor
 *      BRIEF: function that sents file (in bytes) to client for transfer
 *      RETURN: None
 */
void download_file(char* file_passed, int sockfd);

/**
 * VOID UPLOAD_FILE:
 *      PARAMETER: CHAR* FILE_PASSED - file name received from client for upload
 *      PARAMETER: INT FILE_SIZE - size of file to be uploaded
 *      PARAMETER: INT SOCKFD - client socket file descriptor
 *      BRIEF: received file (in bytes) to be transfered to server from client
 *      RETURN: None
 */
void upload_file(char* file_passed, int file_size, int sockfd);

/**
 * VOID* SERVER_FUNC:
 *      PARAMETER: None
 *      BRIEF: handles the server functionality to be passed to each thread
 *      RETURN: (void *) NULL on success, err_code 999 on failure
 */
void* server_func();

/**
 * VOID END_CONNECTION:
 *      PARAMETER: None
 *      BRIEF: handles the graceful termination of a client connection
 *      RETURN: None
 */
void end_connection();

/**
 * BOOL IP_IS_VALID - OPTIONAL:
 *      PARAMETER: CHAR* IP_ADDR - ip address passed to be used by server for network
 *                                  connection
 *      BRIEF: meant to determine if the ip to be used is valid
 *      RETURN: (bool) true/false value whether ip address is valid
 */
bool ip_is_valid(char* ip_addr);

/**
 * INT INIT_THREADPOOL:
 *      PARAMETER: THREADPOOL* POOL - pointer to the pool struct
 *      PARAMETER: SIZE_T NUM_OF_CLIENTS - number of current connections
 *      BRIEF: initializes threadpool holding specified number of threads setting
 *              them to an idle state waiting for handoff to the client
 *      RETURN: (int) 0 on success, -1 on error
 */
int init_threadpool(threadpool* pool, size_t num_of_clients);

/**
 * VOID HANDLE_INCOMING_CLIENT:
 *      PARAMETER: INT CLIENT_FD - client socket file descriptor
 *      BRIEF: obtain socket fd from queue and hand it off to thread for
 *              for network connection to server
 *      RETURN: None
 */
void handle_incoming_client(int client_fd);

/**
 * VOID POOL_CLEANUP:
 *      PARAMETER: SIZE_T THREADS - number of actual threads created and connected
 *      BRIEF: joins all active threads and cleans up resources for proper clean up
 *      RETURN: None
 */
void pool_cleanup(size_t threads);

#endif
