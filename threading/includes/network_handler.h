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
 * @brief - a struct to store server specific data, the port received from
 *          the command line upon server file execution and the total number
 *          of allowable connections (either passed at cmdline or specified)
 *          upon execution
 * @member port - a string representation of the port passed by the user to setup
 *                the TCP client connections
 * @member num_allowable_clients - received either from the command line or within
 *                                 setup 
 */
typedef struct setup_info
{
    char   * port;
    size_t   num_allowable_clients;
} setup_info_t;

/**
 * SETUP_INFO_T * HANDLE_SETUP:
 * @brief - handles the command line arguments passed upon server execution
 *          parses the command line arguments and stores the data appropriately
 * @param argc - the number of command line arguments present
 * @param argv - the array storing the actual command line arguments passed
 * @return - a pointer to the server setup struct containing the port and the
 *           number of allowable clients or NULL on error
 */
setup_info_t * handle_setup(int argc, char ** argv);

/**
 * STRUCT ADDRINFO SETUP_SERVER:
 * @brief - meant to setup the addrinfo struct storing the server's structure data
 * @param p_serv - a pointer the server's addrinfo struct
 * @return - returns a pointer to the server structure set up with ai_family, ai_socktype, 
 *           and ai_flags set
 */
struct addrinfo setup_server (struct addrinfo * p_serv);

/**
 * INT SETUP_SOCKET:
 * @brief - a helper function that sets up the server's TCP socket information
 * @param p_serv - a pointer to the addrinfo struct storing the server's data
 * @param p_port - a string representation of the port used for the socket connection
 *                 upon initial execution of the server program (passed as cmdline argument)
 * @return - returns the working TCP socket file descriptor on success or -1 on failure
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