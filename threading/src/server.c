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

int recv_upload_sz (int sock_fd)
{
    if (-1 == sock_fd)
    {
        errno = EBADF;
        fprintf(stderr, "%s client file descriptor passed is invalid: %s\n", __func__, strerror(errno));
        return -1;
    }

    ssize_t bytes_recv = -1;
    int     file_sz    = -1;

    bytes_recv = recv(sock_fd, &file_sz, sizeof(int), 0);
    if (-1 == bytes_recv)
    {
        errno = EBADF;
        fprintf(stderr, "%s could not recv size of file to upload: %s\n", __func__, strerror(errno));
        return -1;
    }

    if (ERROR == file_sz)
    {
        return -1;
    }

    return file_sz;
}

upload_file_t * recv_upload_fname (int sock_fd)
{
    if (-1 == sock_fd)
    {
        errno = EBADF;
        fprintf(stderr, "%s client file descriptor passed is invalid: %s\n", __func__, strerror(errno));
        return NULL;
    }

    ssize_t bytes_recv = -1;
    upload_file_t * p_file = calloc(1, sizeof(upload_file_t));
    if (NULL == p_file)
    {
        errno = ENOMEM;
        fprintf(stderr, "%s could not allocate memory for upload file data: %s\n", __func__, strerror(errno));
        return NULL;
    }

    bytes_recv = recv(sock_fd, &p_file->name_len, sizeof(int), 0);
    if (-1 == bytes_recv)
    {
        errno = EBADF;
        fprintf(stderr, "%s could not recv filename data from client: %s\n", __func__, strerror(errno));
        CLEAN(p_file);
        return NULL;
    }

    bytes_recv = recv(sock_fd, p_file->filename, p_file->name_len, 0);
    if (-1 == bytes_recv)
    {
        errno = EBADF;
        fprintf(stderr, "%s could not recv filename data from client: %s\n", __func__, strerror(errno));
        CLEAN(p_file);
        return NULL;
    }

    return p_file;
}

int send_data_to_client (int sock_fd, int operation)
{
    if (-1 == sock_fd)
    {
        errno = EBADF;
        fprintf(stderr, "%s client file descriptor passed is invalid: %s\n", __func__, strerror(errno));
        return -1;
    }

    ssize_t bytes_recv = -1;
    char    client_msg[MAX_STR_LEN] = { 0 };

    switch (operation)
    {
        case LIST:
            bytes_recv = recv(sock_fd, client_msg, sizeof(client_msg), 0);
            if (-1 == bytes_recv)
            {
                errno = EBADF;
                fprintf(stderr, "%s could not receive command from the client: %s\n", __func__, strerror(errno));
                return -1;
            }

            if (0 == (strncmp(client_msg, LIST_DIR, strlen(LIST_DIR))))
            {
                fprintf(stdout, "sending directory list to client...\n");
                list_dir(sock_fd);
            }
            else
            {
                fprintf(stderr, "%s invalid directory list command received\n", __func__);
                return -1;
            }
            break;

        case DOWNLOAD:
            bytes_recv = recv(sock_fd, client_msg, sizeof(client_msg), 0);
            if (-1 == bytes_recv)
            {
                errno = EBADF;
                fprintf(stderr, "%s could not received download command from client: %s\n", __func__, strerror(errno));
                return -1;
            }
            else if (true == is_file(client_msg))
            {
                printf("OK\n");
                printf("Sending client %s contents ...\n", client_msg);
                download_file(client_msg, sock_fd);
            }
            else if (0 > *client_msg)
            {
                printf("Client has terminated download process\n");
                printf("Disconnecting client\n");
                return -1;
            }
            else
            {
                download_file(client_msg, sock_fd);
            }
            break;

        default:
            fprintf(stderr, "%s invalid operation received from client\n", __func__);
            return -1;
    }

    return 0;   
}

void * server_func ()
{
    // local variables
    int command = 0;
    char client_msg[MAX_STR_LEN] = { 0 };
    int err_code = 999;
    int * p_err = &err_code;
    ssize_t bytes_recv = 0;
    ssize_t msg_recv = 0;
    int upload_file_sz = 0;
    int fd = 0;
    int ret_val = -1;
    upload_file_t * p_file = NULL;

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

            switch(command)
            {
                // command = 100, send dir list to client
                case(LIST):
                    ret_val = send_data_to_client(fd, command);
                    if (-1 == ret_val)
                    {
                        command = -1;
                        break;
                    }
                    break;

                // command = 200, prepare file for download
                case(DOWNLOAD):
                    ret_val = send_data_to_client(fd, command);
                    if (-1 == ret_val)
                    {
                        command = -1;
                        break;
                    }
                    break;

                // command = 300, prepare server for file upload
                case(UPLOAD):
                    if (UPLOAD == command)
                    {
                        msg_recv = recv(fd, client_msg, sizeof(client_msg), 0);
                        if (0 == (strncmp(client_msg, UPLOAD_FILE, strlen(client_msg))))
                        {
                            printf("upload request received\n");
                            upload_file_sz = recv_upload_sz(fd);
                            if (-1 == upload_file_sz)
                            {
                                command = -1;
                                upload_file(NULL, -1, fd);
                                break;
                            }
                            
                            printf("Uploading file of size %d from client\n", upload_file_sz);
                            p_file = recv_upload_fname(fd);
                            if (NULL == p_file)
                            {
                                command = -1;
                                upload_file(NULL, -1, fd);
                                break;
                            }
                            else
                            {
                                printf("File name received: %s\n", p_file->filename);
                                upload_file(p_file->filename, upload_file_sz, fd);
                                CLEAN(p_file);
                            }
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

struct pollfd * setup_poll (int server_fd, int fd_size)
{
    if ((-1 == server_fd) || (fd_size < 1))
    {
        errno = EINVAL;
        fprintf(stderr, "%s() - one or more parameters passed are invalid: %s\n", __func__, strerror(errno));
        return NULL;
    }

    struct pollfd * pfds = calloc(fd_size, sizeof(*pfds));
    if (NULL == pfds)
    {
        errno = ENOMEM;
        fprintf(stderr, "%s could not allocate pfds array: %s\n", __func__, strerror(errno));
        close(server_fd);
        return NULL;
    }

    pfds[0].fd      = server_fd;
    pfds[0].events  = POLLIN;

    return pfds;
}

int main (int argc, char** argv)
{
    struct sigaction ig_SIGINT = { 0 };
    ig_SIGINT.sa_handler = handle_signal;
    if (0 > sigaction(SIGINT, &ig_SIGINT, NULL))
    {
        exit(-1);
    }

    int sockfd = 0;
    int ret_val = -1;

    ret_val = init_globals();
    if (-1 == ret_val)
    {
        fprintf(stderr, "%s failed to initialize all global data\n", __func__);
        return EXIT_FAILURE;
    }

    setup_info_t * p_setup = handle_setup(argc, argv);
    ret_val = init_threadpool(p_tpool, p_setup->num_allowable_clients);
    if (-1 == ret_val)
    {
        fprintf(stderr, "%s failed to initialize client threadpool\n", __func__);
        return EXIT_FAILURE;
    }

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
        char * p_addr = SERV_ADDR;
        printf("Server listening on - %s:%s\n", p_addr, p_setup->port);
    }

    int timeout     = 60000;
    int fd_count    = 1;
    int fd_size     = MAXQUEUE;

    struct pollfd * pfds = setup_poll(sockfd, fd_size);
    if (NULL == pfds)
    {
        goto CLEANUP;
    }

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
