#ifndef __NETWORK_HANDLER_H__
#define __NETWORK_HANDLER_H__

#define _XOPEN_SOURCE 700
#define _GNU_SOURCE

#include <stdio.h>
#include <getopt.h>
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

#include "global_data.h"

#define MAX_PORT_LEN 6
#define BASE_10      10
#define MIN_PORT     1025
#define MAX_PORT     65535

/**
 * 
 */
typedef struct setup_info
{
    char   * port;
    size_t   num_allowable_clients;
} setup_info_t;

/**
 * 
 */
setup_info_t * handle_setup(int argc, char ** argv);

/**
 * 
 */
struct addrinfo setup_server (struct addrinfo * p_serv);

/**
 * 
 */
int setup_socket (struct addrinfo * p_serv, char * p_port);

/**
 * VOID HANDLE_INCOMING_CLIENT:
 * @brief - obtain socket fd from queue and hand it off to thread for
 *          for network connection to server
 * @param client_fd - client socket file descriptor
 * @return - N/A
 */
void handle_incoming_client (int client_fd);

/**
 * VOID END_CONNECTION:
 * @brief - handles the graceful termination of a client connection
 * @param - N/A
 * @return - N/A
 */
void end_connection ();



#endif