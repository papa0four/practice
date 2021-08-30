#define _XOPEN_SOURCE 700
#define _GNU_SOURCE
#include "../includes/server.h"

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

int main (int argc, char** argv)
{
    struct sigaction ig_SIGINT = { 0 };
    ig_SIGINT.sa_handler = handle_signal;
    if (0 > sigaction(SIGINT, &ig_SIGINT, NULL))
    {
        exit(-1);
    }

    // main variables
    int sockfd = 0;
    int ret_val = -1;

    ret_val = init_globals();
    if (-1 == ret_val)
    {
        fprintf(stderr, "%s failed to initialize all global data\n", __func__);
        return EXIT_FAILURE;
    }

    setup_info_t * p_setup = handle_setup(argc, argv);

    printf("port passed: %s\n", p_setup->port);
	char * p_addr = SERV_ADDR;

    init_threadpool(p_tpool, p_setup->num_allowable_clients);

    struct addrinfo p_server = { 0 };
    p_server = setup_server(&p_server);
    struct sockaddr_storage cli_address = { 0 };
    socklen_t cli_addr_len = sizeof(cli_address);

    sockfd = setup_socket(&p_server, p_setup->port);
    if ((listen(sockfd, MAXQUEUE)) != 0)
    {
        perror("listen call failed");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Server listening on - %s:%s\n", p_addr, p_setup->port);
    }

    int timeout     = 60000;
    int fd_count    = 1;
    int fd_size     = MAXQUEUE;

    struct pollfd * pfds = calloc(fd_size, sizeof(*pfds));
    if (NULL == pfds)
    {
        errno = ENOMEM;
        fprintf(stderr, "%s could not allocate pfds array: %s\n", __func__, strerror(errno));
        goto CLEANUP;
    }

    pfds[0].fd      = sockfd;
    pfds[0].events  = POLLIN;

    while (serv_running)
    {

        printf("Number of total clients allowed: %ld\n", p_setup->num_allowable_clients);
        printf("Number of current connections to server: %ld\n", clients_con);

        int poll_count = poll(pfds, fd_count, timeout);
        if (-1 == poll_count)
        {
            CLEAN(pfds);
            goto CLEANUP;
        }

        for (int idx = 0; idx < fd_count; idx++)
        {
            if (pfds[idx].revents & POLLIN)
            {
                if (sockfd == pfds[idx].fd)
                {
                    int connfd = accept(sockfd, (struct sockaddr *)&cli_address, &cli_addr_len);
                    if (-1 == connfd)
                    {
                        printf("Cleaning up resources\n");
                        CLEAN(pfds);
                        goto CLEANUP;
                    }
                    else
                    {
                        clients_con++;
                        handle_incoming_client(connfd);

                        if (false == serv_running)
                        {
                            pool_cleanup(p_setup->num_allowable_clients);
                            break;
                        }
                        /**
                         * if more clients than allowed connect
                         * decrease the global back to max allowed
                         * close the socket to prevent further connections
                         * this will not terminate the current authorized connections
                         */
                        if (clients_con > p_setup->num_allowable_clients)
                        {
                            clients_con--;
                            close(sockfd);
                        }
                    }
                }
            }
        }
    }

    close(sockfd);
    CLEAN(pfds);
    CLEAN(p_setup->port);
    CLEAN(p_setup);
    CLEAN(p_queue);
    return EXIT_SUCCESS;

CLEANUP:
close(sockfd);
pool_cleanup(p_setup->num_allowable_clients);
CLEAN(p_setup->port);
CLEAN(p_setup);
CLEAN(p_queue);
return EXIT_FAILURE;

}
