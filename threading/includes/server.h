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
 * BOOL IP_IS_VALID - OPTIONAL:
 * @brief - meant to determine if the ip to be used is valid
 * @param p_ipaddr - ip address passed to be used by server for network
 *                                  connection
 * @return - (bool) true/false value whether ip address is valid
 */
bool ip_is_valid (char * p_ipaddr);

#endif
