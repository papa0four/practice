#define _XOPEN_SOURCE 700
#define _GNU_SOURCE
#include "../includes/server.h"

/* global variables */
bool            serv_running = true;
bool            exiting      = false;
size_t          clients_con  = 0;
threadpool    * p_tpool      = NULL;
queue         * p_queue      = NULL;

void handle_signal (int sig_no)
{
    if (SIGINT == sig_no)
    {
        printf("\nCTRL + c caught, shutting down, Goodbye...\n");
        pthread_mutex_lock(&p_tpool->lock);
        serv_running = false;
        pthread_mutex_unlock(&p_tpool->lock);
    }
}

void end_connection()
{
    pthread_mutex_lock(&p_tpool->lock);
    p_tpool->thread_cnt--;
    pthread_mutex_unlock(&p_tpool->lock);
    clients_con--;
    printf("Number of current connections: %ld\n", clients_con);
}

void * server_func ()
{
    // local variables
    int command = 0;
    char client_msg[MAX_STR_LEN] = { 0 };
    char * p_filename = NULL;
    int err_code = 999;
    int * p_err = &err_code;
    ssize_t bytes_recv = 0;
    ssize_t msg_recv = 0;
    ssize_t sz_recv = 0;
    int name_len = 0;
    ssize_t file_len_recv = 0;
    ssize_t file_recv = 0;
    int upload_file_sz = 0;
    int fd = 0;

    while (serv_running)
    {
        pthread_mutex_lock(&p_tpool->lock);
        while (serv_running && is_empty(p_queue))
        {
            pthread_cond_wait(&p_tpool->condition, &p_tpool->lock);
            if (false == serv_running)
            {
                pthread_mutex_unlock(&p_tpool->lock);
                pthread_cond_broadcast(&p_tpool->condition);
                return (void *) p_err;
            }
        }
        
        fd = dequeue(p_queue);
        p_tpool->thread_cnt++;
        pthread_mutex_unlock(&p_tpool->lock);
        pthread_cond_broadcast(&p_tpool->condition);

        while (serv_running)
        {
            bytes_recv = recv(fd, &command, sizeof(int), 0);
            if (0 > bytes_recv)
            {
                errno = EINVAL;
                perror("No message received from client");
                break;
            }

            printf("Received from client: %d\n", command);
            switch(command)
            {
                // command = 100, send dir list to client
                case(LIST):
                    if (LIST == command)
                    {
                        msg_recv = recv(fd, client_msg, sizeof(client_msg), 0);

                        printf("CLIENT_MSG: %s\n", client_msg);
                        if (0 == (strncmp(client_msg, "ls", strlen(client_msg))))
                        {
                            printf("Sending directory list...\n");
                            list_dir(fd);
                        }
                        else
                        {
                            printf("Invalid list command received\n");
                        }
                    }
                    break;

                // command = 200, prepare file for download
                case(DOWNLOAD):
                    if (DOWNLOAD == command)
                    {
                        msg_recv = recv(fd, client_msg, sizeof(client_msg), 0);
                        if (true == is_file(client_msg))
                        {
                            printf("OK\n");
                            printf("Sending client %s contents ...\n", client_msg);
                            download_file(client_msg, fd);
                        }
                        else if (0 > *client_msg)
                        {
                            printf("Client has terminated download process\n");
                            printf("Disconnecting client\n");
                            command = 500;
                        }
                        else
                        {
                            download_file(client_msg, fd);
                        }
                    }
                    break;

                // command = 300, prepare server for file upload
                case(UPLOAD):
                    if (UPLOAD == command)
                    {
                        msg_recv = recv(fd, client_msg, sizeof(client_msg), 0);
                        if (0 == (strncmp(client_msg, "upload", strlen(client_msg))))
                        {
                            printf("upload request received\n");
                            sz_recv = recv(fd, &upload_file_sz, sizeof(int), 0);
                            if (0 == sz_recv)
                            {
                                upload_file_sz = -1;
                                command = 666;
                                goto END;
                            }
                            else if (666 == upload_file_sz || 500 == upload_file_sz)
                            {
                                upload_file_sz = -1;
                                goto END;
                            }
                            
                            printf("Uploading file of size %d from client\n", upload_file_sz);
                            file_len_recv = recv(fd, &name_len, sizeof(int), 0);
                            if (0 >= file_len_recv)
                            {
                                errno = EINVAL;
                                perror("Upload file name size not received");
                                break;
                            }

                            p_filename = calloc((name_len + 1), sizeof(char));
                            if (NULL == p_filename)
                            {
                                errno = ENOMEM;
                                perror("Could not allocate memory for p_filename");
                                break;
                            }

                            file_recv = recv(fd, p_filename, name_len, 0);
                            if (0 >= file_recv)
                            {
                                errno = EINVAL;
                                perror("Upload file name not received");
                                CLEAN(p_filename);
                                break;
                            }
                            printf("File name received: %s\n", p_filename);
                            END:
                                upload_file(p_filename, upload_file_sz, fd);
                                CLEAN(p_filename);
                        }
                    }
                    break;

                // command = 500, graceful client exit from connection
                case(EXIT):
                    if (EXIT == command)
                    {
                        msg_recv = recv(fd, client_msg, sizeof(client_msg), 0);
                        if (0 == (strncmp(client_msg, "exit", strlen(client_msg))))
                        {
                            exiting = true;
                            command = -1;
                        }
                        printf("Client has ended the connection...\n");
                    }
                    break;

                // if invalid command is received from client
                default:
                    printf("Invalid command received from the client\n");
                    command = EXIT;
                    break;
            }

            memset(client_msg, 0, sizeof(client_msg));
            if (command == -1)
            {
                break;
            }
        }

		if (0 >= bytes_recv || 0 >= msg_recv || true == exiting)
		{
			end_connection();
		}
    }
 
    return NULL;
}

int init_threadpool(threadpool* p_tpool, size_t num_of_clients)
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

    for (size_t i = 0; i < num_of_clients; i++)
    {
        if (0 != pthread_create(&p_tpool->threads[i], NULL, server_func, NULL))
        {
            errno = EINVAL;
            perror("Error creating thread");
        }
    }

    return 0;
}

void handle_incoming_client(int client_fd)
{
    item * q_item = NULL;
    q_item = calloc(1, sizeof(item));
    if (NULL == q_item)
    {
        errno = ENOMEM;
        perror("Could not allocate memory for q_item");
        exit(-1);
    }
    q_item->data = client_fd;
    pthread_mutex_lock(&p_tpool->lock);
    enqueue(p_queue, *q_item);
    pthread_mutex_unlock(&p_tpool->lock);
    pthread_cond_broadcast(&p_tpool->condition);

    CLEAN(q_item);
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

int main (int argc, char** argv)
{
    // main variables
    int sockfd = 0;
    int opt = 0;
    int port = 0;
    const int enable = 1;
    size_t thread_cnt = 0;
    size_t num_of_clients = 0;
    struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;
	socklen_t cli_addr_len;

    if (1 >= argc)
    {
        // set errno value and error message
        errno = EINVAL;
        perror("Invalid arguments passed\nRequired Argument\n\t-p [SRV_PORT]:"
				"Optional Argument\n\t-t [MAX_ALLOWED_CLIENTS]\n");
        exit(EXIT_FAILURE);
    }

    while ((opt = getopt(argc, argv, ":p:t:")) != -1)
    {
        switch(opt)
        {
            case 'p':
                port = strtol(optarg, NULL, 10);
                if (1025 > port || 65535 < port)
                {
                    errno = EINVAL;
                    // create error message
                    perror("Invalid port argument passed\n"
                    "Valid port range: 1025 - 65535");
                    exit(EXIT_FAILURE);
                }
                break;

            case 't':
                thread_cnt = strtol(optarg, NULL, 10);
                if (1 > thread_cnt)
                {
                    errno = EINVAL;
                    perror("invalid thread number passed, must be >= 1");
                    exit(EXIT_FAILURE);
                }

                num_of_clients = thread_cnt;

                if (MAX_CLIENTS < num_of_clients)
                {
                    printf("Server cannot contain more than %d clients\n"
                            "Setting default to: %d\n", MAX_CLIENTS, MAX_CLIENTS);
                    num_of_clients = MAX_CLIENTS;
                }
				else if ('?' == *optarg)
				{
					printf("No client capacity set: setting default to %d\n", MAX_CLIENTS);
					num_of_clients = MAX_CLIENTS;
				}
                break;

            default:
                printf("Invalid cmdline argument passed\nRequired Argument\n\t-p [SRV_PORT]:\n"
                        "Optional Argument\n\t-t [CLIENT_AMOUNT] (argument optional)\n");
                exit(-1);
        }
    }

    struct sigaction ig_SIGINT = { 0 };
    ig_SIGINT.sa_handler = handle_signal;
    if (0 > sigaction(SIGINT, &ig_SIGINT, NULL))
    {
        exit(-1);
    }

	memset(&cli_addr, 0, sizeof(cli_addr));

	char * p_addr = SERV_ADDR;

    p_queue = init_queue();
    p_tpool = calloc(1, sizeof(threadpool));
    if (NULL == p_tpool)
    {
        errno = ENOMEM;
        perror("Could not allocat memory for p_tpool in main");
        exit(EXIT_FAILURE);
    }

    init_threadpool(p_tpool, num_of_clients);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sockfd)
    {
        errno = ENOTSOCK;
        perror("Could not create socket");
        exit(EXIT_FAILURE);
    }

    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1)
    {
        errno = ENOTSOCK;
        perror("Could not set socket option");
        exit(EXIT_FAILURE);
    }

	memset(&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(p_addr);
	serv_addr.sin_port = htons(port);

    if ((bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) != 0)
    {
        perror("Could not bind on socket");
        exit(EXIT_FAILURE);
    }

    if ((listen(sockfd, 1)) != 0)
    {
        perror("listen call failed");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Server listening on - %s:%d\n", p_addr, port);
    }

    cli_addr_len = sizeof(cli_addr);

    while (serv_running)
    {

        printf("Number of total clients allowed: %ld\n", num_of_clients);
        printf("Number of current connections to server: %ld\n", clients_con);

        int connfd = accept(sockfd, (struct sockaddr *)&cli_addr, &cli_addr_len);
        if (-1 == connfd)
        {
            printf("Cleaning up resources\n");
            pool_cleanup(num_of_clients);
            goto END;
        }

        clients_con++;
        
        handle_incoming_client(connfd);

        if (false == serv_running)
        {
            pool_cleanup(num_of_clients);
            break;
        }
        /**
         * if more clients than allowed connect
         * decrease the global back to max allowed
         * close the socket to prevent further connections
         * this will not terminate the current authorized connections
         */
        if (clients_con > num_of_clients)
        {
            clients_con--;
            close(sockfd);
        }
    }
    // gracefully shuts down the socket and cleans up server resources
    END:
        close(sockfd);
        CLEAN(p_queue);
        return EXIT_SUCCESS;
}
