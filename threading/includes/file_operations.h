#ifndef __FILE_OPERATIONS_H__
#define __FILE_OPERATIONS_H__

#define _XOPEN_SOURCE 700
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <inttypes.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/socket.h>

#include "global_data.h"

#define MAX_STR_LEN     255

/**
 * INT GET_FILE_COUNT:
 * @brief - determines the number of files located within the file server
 * @param sockfd - client socket file descriptor
 * @return - (int) number of DT_REG files in FileServer directory, -1 on error
 */
int get_file_count (int sockfd);

/**
 * VOID LIST_DIR:
 * @brief - creates a list of DT_REG files to send to client (similar to basic ls cmd)
 * @param sockfd - client socket file descriptor
 * @return - N/A
 */
void list_dir(int sockfd);

/**
 * BOOL IS_FILE:
 * @brief - determine whether passed file exists within the directory
 * @param p_filename - name of file for validity check
 * @return - true/false value whether file exists
 */
bool is_file(char * p_filename);

/**
 * VOID DOWNLOAD_FILE:
 * @brief - function that sents file (in bytes) to client for transfer
 * @param p_file_passed - file within server to be sent to client
 * @param sockfd - client socket file descriptor
 * @return - N/A
 */
void download_file(char * p_file_passed, int sockfd);

/**
 * VOID UPLOAD_FILE:
 * @brief - received file (in bytes) to be transfered to server from client
 * @param p_file_passed - file name received from client for upload
 * @param file_size - size of file to be uploaded
 * @param sockfd - client socket file descriptor
 * @return - N/A
 */
void upload_file(char * p_file_passed, int file_size, int sockfd);

#endif