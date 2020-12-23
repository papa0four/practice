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

typedef struct _thread_pool threadpool;
struct dirent* dir;

typedef struct _thread_pool {
    pthread_t* threads;
    size_t max_thread_cnt;
    volatile int thread_cnt;
    pthread_mutex_t lock;
    pthread_cond_t condition;
}threadpool;

void handle_signal(int sig_no);
int get_file_count(int sockfd);
void list_dir(int sockfd);
bool is_file(char* file_name);
void download_file(char* file_passed, int sockfd);
void upload_file(char* file_passed, int file_size, int sockfd);
void* server_func();
void end_connection();
bool ip_is_valid(char* ip_addr);
int init_threadpool(threadpool* pool, size_t num_of_clients);
void handle_incoming_client(int client_fd);
void pool_cleanup();

#endif
