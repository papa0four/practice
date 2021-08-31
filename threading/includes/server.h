#ifndef __SERVER_H__
#define __SERVER_H__

#define _XOPEN_SOURCE 700
#define _GNU_SOURCE

#include <stdio.h>
#include <netdb.h>
#include <poll.h>
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

#include "my_queue.h"
#include "file_operations.h"
#include "network_handler.h"

#define LIST            100
#define DOWNLOAD        200
#define UPLOAD          300
#define EXIT            500
#define ERROR           -1

#define LIST_DIR        "ls"
#define UPLOAD_FILE     "upload"

typedef struct upload_file
{
    int         name_len;
    char        filename[MAXNAMLEN];
} upload_file_t;



/**
 * VOID HANDLE_SIGNAL:
 * @brief - handle ctrl+c input from user
 * @param sig_no: integer representation for SIGINT
 * @return - N/A
 */
void handle_signal (int sig_no);

/**
 * VOID * SERVER_FUNC:
 * @brief - handles the server functionality to be passed to each thread
 * @param - N/A
 * @return - (void *) NULL on success, err_code 999 on failure
 */
void * server_func ();

/**
 * INT RECV_UPLOAD_SZ:
 * @brief - if the client requests to upload a file to the file server, this
 *          function should receive the size of the file meant to be uploaded
 * @param sock_fd - the client's socket file descriptor
 * @return - 0 on success, -1 on error
 */
int recv_upload_sz (int sock_fd);

/**
 * UPLOAD_FILE_T * RECV_UPLOAD_FNAME:
 * @brief - receives the name length and the file name meant to be uploaded to the
 *          file server
 * @param sock_fd - the client's socket file descriptor
 * @return - a pointer to the struct storing the file name data (i.e. name length and
 *           the filename itself) or NULL on failure
 */
upload_file_t * recv_upload_fname (int sock_fd);

/**
 * INT SEND_DATA_TO_CLIENT:
 * @brief - this function sends the appropriate data to the client if a directory list or
 *          download request is received from the client
 * @param sock_fd - the client's socket file descriptor
 * @param operation - the operation to be performed (list or download)
 * @return - 0 on success or -1 on error 
 */
int send_data_to_client (int sock_fd, int operation);

/**
 * STRUCT POLLFD * SETUP_POLL:
 * @brief - sets up the poll structure to handle incoming client socket connections
 * @param server_fd - the server's socket file descriptor
 * @param fd_size - the max allowable number of client connections
 */
struct pollfd * setup_poll (int server_fd, int fd_size);

#endif
