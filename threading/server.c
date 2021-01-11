#define _XOPEN_SOURCE 700
#define _GNU_SOURCE
#include "my_queue.h"
#include "server.h"

#define SERV_ADDR "127.0.0.1"
#define MAX_STR_LEN 255
#define MAX_CLIENTS 50
#define LIST 100
#define DOWNLOAD 200
#define UPLOAD 300
#define EXIT 500

/* global variables */
bool serv_running = true;
bool exiting = false;
size_t clients_con = 0;
threadpool* pool = NULL;
queue* fd_queue = NULL;

/**
 * catch ctrl+c call while the server is running
 * while there are clients connected, the server cannot be killed
 * if no clients are connected, the server can be killed
 */
void handle_signal(int sig_no)
{
    if (SIGINT == sig_no)
    {
        // goodbye message
        printf("\nGoodbye...\n");
        // lock shared resources
        pthread_mutex_lock(&pool->lock);
        // switch global server running variable
        serv_running = false;
        // unlock shared resources
        pthread_mutex_unlock(&pool->lock);
    }
} // end handle signal

/**
 * helper function to get the amount of files
 * contained within the File Server
 * sends valid files to client and returns number of files
 */
int get_file_count(int sockfd)
{
    // check if file descriptor is valid
    if (0 > sockfd)
    {
        return 0;
    }
    // local variables
    ssize_t bytes_sent = 0;
    int file_count = 0;
    DIR* d = NULL;
    // call open dir on File directory
    d = opendir("FileServer/");
    if (d)
    {
        // loop through directory
        while (NULL != (dir = readdir(d)))
        {
            // check for regular file type
            if (DT_REG == dir->d_type)
            {
                // increment number of regular files
                file_count++;
            }
            else
            {
                // if not regular file, continue loop
                continue;
            }
        }
    }
    closedir(d);
    // send number of refular files to client
    bytes_sent = send(sockfd, &file_count, sizeof(int), 0);
    if (0 > bytes_sent)
    {
        printf("Error sending file count\n");
        return -1;
    }
    return file_count;
} // end get file count

/**
 * get directory listing of regular files
 * send directory list to client
 * this allows the client to see what is contained in the server
 */
void list_dir(int sockfd)
{
    // local variables
    ssize_t bytes_sent = 0;
    ssize_t name_sent = 0;
    DIR* d = NULL;
    int num_files = get_file_count(sockfd);
    size_t name_len = 0;
    // allocate memory for file list
    char** file_list = NULL;
    file_list = calloc(num_files, sizeof(char*));
    if (NULL == file_list)
    {
        errno = ENOMEM;
        perror("Could not allocate memory for file_list");
        exit(EXIT_FAILURE);
    }
    // open directory and check for error
    if ((d = opendir("FileServer/")) == NULL)
    {
        return;
    }
    // iterator variable for 2D array index
    int i = 0;
    // loop over directory
    while (NULL != (dir = readdir(d)) && num_files >= i)
    {
        // if file type is "regular"
        if (DT_REG == dir->d_type)
        {
            // allocate each array index for file names within directory
            file_list[i] = calloc(1, (sizeof(char) * MAX_STR_LEN));
            if (NULL == file_list[i])
            {
                errno = ENOMEM;
                perror("Could not allocate memory for file");
                exit(EXIT_FAILURE);
            }

            // copy file name from readdir to 2D file list
            memcpy(file_list[i], dir->d_name, strlen(dir->d_name));
            // get the length of each file name
            name_len = strnlen(file_list[i], MAX_STR_LEN);
            // send name length to the client
            bytes_sent = send(sockfd, &name_len, sizeof(size_t), 0);
            if (0 > bytes_sent)
            {
                printf("Error sending file name length\n");
                return;
            }
            // send the file names to the client
            name_sent = send(sockfd, file_list[i], strnlen(file_list[i], MAX_STR_LEN), 0);
            if (0 > name_sent)
            {
                printf("Error sending directory list\n");
                return;
            }
            i++;
        }
        else
        {
            // if not regular file type, continue loop
            continue;
        }
    }
    closedir(d);
    // clean up allocated memory for 2D array
    for (int i = 0; i < num_files; i++)
    {
        free(file_list[i]);
        file_list[i] = NULL;
    }
    free(file_list);
    file_list = NULL;
} // end list dir

/**
 * check if file received from client exists on the server
 * bool function that performs a validity check
 * taking file name received from the client and
 * determines if it exists within the FileServer directory
 */
bool is_file(char* file_name)
{
    // FileServer directory
    char file_path[] = "FileServer/";
    // allocate memory to create absolute file path
    /**
     * fix memory allocation here
     * seems to be an error
     */
    char* full_path = NULL;
    full_path = calloc(1, (strlen(file_path) + strlen(file_name)) + 1);
    if (NULL == full_path)
    {
        errno = ENOMEM;
        perror("Could not allocate memory for full path");
        exit(-1);
    }
    // copy direcotry into full path array
    strncpy(full_path, file_path, strlen(file_path));
    // append file passed from client into absolute path
    strncat(full_path, file_name,strlen(file_name));
    // call access function to determine file existence
    if (access(full_path, F_OK) == 0)
    {
        free(full_path);
        full_path = NULL;
        return true;
    }
    // if false free resources and return false
    free(full_path);
    full_path = NULL;
    return false;
} // end is file

/**
 * receive a file from client to download from server to client
 * calls the validity check before allowing client to download
 * reads the file contents and sends to client in chunks
 * before rewinding the file pointer
 */
void download_file(char* file_passed, int sockfd)
{
    // local variables
    FILE* fp = NULL;
    char file_path[] = "FileServer/";
    signed int err_code = -1;
    char* buffer = NULL;
    // allocate memory for absolute path
    char* full_path = NULL;
    full_path = calloc(1, (strlen(file_path) + strlen(file_passed)) + 1);
    if (NULL == full_path)
    {
        errno = ENOMEM;
        perror("Could not allocate memory for full path");
        return;
    }
    // copy directory into absolute path array
    strncpy(full_path, file_path, strlen(file_path));
    // append the file received from client to absolute path
    strncat(full_path, file_passed,strlen(file_passed));
    // open the file to read in binary mode
    fp = fopen(full_path, "rb");
    if (NULL == fp)
    {
        errno = ENOENT;
        perror("Could not open file passed");
        send(sockfd, &err_code, sizeof(int), 0);
        free(full_path);
        full_path = NULL;
        return;
    }
    // seek file pointer to end to obtain file size
    if (fseek(fp, 0L, SEEK_END) == 0)
    {
        // set variable to file size
        long file_sz = ftell(fp);
        if (-1 == file_sz)
        {
            perror("ftell error on file");
            free(full_path);
            full_path = NULL;
            return;
        }
        // allocate memory to store file contents into array
        buffer = calloc(file_sz, sizeof(char));
        if (NULL == buffer)
        {
            errno = ENOMEM;
            perror("Could not allocate memory for buffer");
            free(full_path);
            full_path = NULL;
            return;
        }
        // rewind file pointer to beginning of file
        if (fseek(fp, 0L, SEEK_SET) != 0)
        {
            perror("Could not rewind file");
            free(full_path);
            full_path = NULL;
            free(buffer);
            buffer = NULL;
            return;
        }
        // set a changeable variable to overall file size
        long bytes_left = file_sz;
        // variable used to read file in chunks
        size_t length = 0;
        // send file size to client
        send(sockfd, &file_sz, sizeof(long), 0);
        printf("Sending file of size %ld to client...\n", file_sz);
        // create a loop to send file contents to client in chunks       
        while (bytes_left > 0)
        {
            length = fread(buffer, sizeof(char), 1024, fp);
            if (ferror(fp) != 0)
            {
                fprintf(stderr, "Error reading file: %s\n", file_passed);
                free(full_path);
                full_path = NULL;
                free(buffer);
                buffer = NULL;
                return;
            }
            // send content chunks to client
            send(sockfd, buffer, length, 0);
            // decrease changeable variable to ensure all bytes are sent
            bytes_left -= 1024;
        }      
    }
    // close file pointer and free allocated memory
    fclose(fp);
    free(full_path);
    full_path = NULL;
    free(buffer);
    buffer = NULL;
} // end download file

void upload_file(char* file_passed, int file_size, int sockfd)
{
    if (-1 == file_size)
    {
        printf("Error received from client\n");
        printf("Upload failed...\n");
        return;
    }
    FILE* fp = NULL;
    char file_path[] = "FileServer/";
    int bytes_recv = 0;
    signed int err_code = -1;
    char* buffer = NULL;
    char* full_path = NULL;
    /**
     * appears to be an issue with the way
     * memory is allocated for the full file path
     * needs to be fixed when running with valgrind
     */
    full_path = calloc(1, (strlen(file_path) + strlen(file_passed)) + 1);
    if (NULL == full_path)
    {
        errno = ENOMEM;
        perror("Could not allocate memory for full path");
        return;
    }
    strncpy(full_path, file_path, strlen(file_path));
    strncat(full_path, file_passed, strlen(file_passed));
    strncat(full_path, "\0", sizeof(char));
    printf("Saving Client File to: %s\n", full_path);
    if (true == is_file(file_passed))
    {
        printf("Overwriting existing file...\n");
        fclose(fopen(full_path, "w"));
    }
    fp = fopen(full_path, "wb+");
    if (NULL == fp)
    {
        errno = ENOENT;
        perror("Could not open file for writing");
        send(sockfd, &err_code, sizeof(int), 0);
        free(full_path);
        full_path = NULL;
        return;
    }
    long bytes_left = file_size;
    buffer = calloc(1, file_size);
    if (NULL == buffer)
    {
        errno = ENOMEM;
        perror("Could not allocate memory for buffer in upload");
        free(full_path);
        full_path = NULL;
    }
    while (bytes_left > 0)
    {
        bytes_recv = recv(sockfd, buffer, sizeof(char), 0);
        if (0 >= bytes_recv)
        {
            errno = EINVAL;
            perror("file data not received");
            free(full_path);
            full_path = NULL;
            free(buffer);
            buffer = NULL;
            return;
        }
        fprintf(fp, "%c", *buffer);
        bytes_left -= 1;
    }
    fclose(fp);
    free(full_path);
    full_path = NULL;
    free(buffer);
    buffer = NULL;
    printf("Upload Complete\n");
}

void end_connection()
{
    pthread_mutex_lock(&pool->lock);
    pool->thread_cnt--;
    pthread_mutex_unlock(&pool->lock);
    clients_con--;
    printf("Number of current connections: %ld\n", clients_con);
}

void* server_func()
{
    int command = 0;
    char client_msg[MAX_STR_LEN] = { 0 };
    char* file_name = NULL;
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
        pthread_mutex_lock(&pool->lock);
        while (serv_running && is_empty(fd_queue))
        {
            pthread_cond_wait(&pool->condition, &pool->lock);
        }
        if (false == serv_running)
        {
            pthread_mutex_unlock(&pool->lock);
            return NULL;
        }
        fd = dequeue(fd_queue);
        pool->thread_cnt++;
        pthread_mutex_unlock(&pool->lock);
        pthread_cond_broadcast(&pool->condition);
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
                    }
                    break;

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
                        else
                        {
                            download_file(client_msg, fd);
                        }
                    }
                    break;

                case(UPLOAD):
                    if (UPLOAD == command)
                    {
                        msg_recv = recv(fd, client_msg, sizeof(client_msg), 0);
                        if (0 == (strncmp(client_msg, "upload", strlen(client_msg))))
                        {
                            printf("upload request received\n");
                            sz_recv = recv(fd, &upload_file_sz, sizeof(int), 0);
                            printf("SZ_RECV: %zd\n", sz_recv);
                            if (0 == sz_recv)
                            {
                                upload_file_sz = -1;
                                command = 666;
                                goto END;
                            }
                            else if (666 == upload_file_sz)
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
                            file_name = calloc(1, (name_len + 1));
                            if (NULL == file_name)
                            {
                                errno = ENOMEM;
                                perror("Could not allocate memory for file_name");
                                break;
                            }
                            file_recv = recv(fd, file_name, name_len, 0);
                            if (0 >= file_recv)
                            {
                                errno = EINVAL;
                                perror("Upload file name not received");
                                break;
                            }
                            printf("File name received: %s\n", file_name);
                            END:
                                upload_file(file_name, upload_file_sz, fd);
                                free(file_name);
                                file_name = NULL;
                        }
                    }
                    break;

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

int init_threadpool(threadpool* pool, size_t num_of_clients)
{
    if (NULL == pool)
    {
        errno = EINVAL;
        perror("Pool passed is NULL");
        return -1;
    }
    pool->threads = calloc(num_of_clients, sizeof(pthread_t));
    if (NULL == pool->threads)
    {
        errno = ENOMEM;
        perror("Could not allocate memory for pool->threads");
        free(pool);
        pool = NULL;
        return -1;
    }
    pool->max_thread_cnt = num_of_clients;
    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->condition, NULL);
    for (size_t i = 0; i < num_of_clients; i++)
    {
        if (0 != pthread_create(&pool->threads[i], NULL, server_func, NULL))
        {
            errno = EINVAL;
            perror("Error creating thread");
        }
    }
    return 0;
}

void handle_incoming_client(int client_fd)
{
    item* q_item = NULL;
    q_item = calloc(1, sizeof(item));
    if (NULL == q_item)
    {
        errno = ENOMEM;
        perror("Could not allocate memory for q_item");
        exit(-1);
    }
    q_item->data = client_fd;
    pthread_mutex_lock(&pool->lock);
    enqueue(fd_queue, *q_item);
    pthread_mutex_unlock(&pool->lock);
    pthread_cond_broadcast(&pool->condition);
    free(q_item);
    q_item = NULL;
}

void pool_cleanup(size_t threads)
{
    for (size_t i = 0; i < threads; i++)
    {
        pthread_cond_broadcast(&pool->condition);
        pthread_join(pool->threads[i], NULL);
    }
    clear(fd_queue);
    free(pool->threads);
    pool->threads = NULL;
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->condition);
    free(pool);
    pool = NULL;
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
                    printf("Server cannot contain more than 10,000 clients\n"
                            "Setting default to: 10,000\n");
                    num_of_clients = MAX_CLIENTS;
                }
				else if ('?' == *optarg)
				{
					printf("No client capacity set: setting default to 100\n");
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
	char* addr = SERV_ADDR;
    fd_queue = init();
    pool = calloc(1, sizeof(threadpool));
    if (NULL == pool)
    {
        errno = ENOMEM;
        perror("Could not allocat memory for pool in main");
        exit(EXIT_FAILURE);
    }
    init_threadpool(pool, num_of_clients);
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
	serv_addr.sin_addr.s_addr = inet_addr(addr);
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
        printf("Server listening on - %s:%d\n", addr, port);
    }
    cli_addr_len = sizeof(cli_addr);
    while (serv_running)
    {
        printf("Number of total clients allowed: %ld\n", num_of_clients);
        printf("Number of current connections to server: %ld\n", clients_con);
        int connfd = accept(sockfd, (struct sockaddr *)&cli_addr, &cli_addr_len);
        clients_con++;
        if (0 > connfd)
        {
            printf("Cleaning up resources\n");
            goto END;
        }
        handle_incoming_client(connfd);
        if (false == serv_running)
        {
            pool_cleanup(num_of_clients);
            break;
        }
        if (clients_con > num_of_clients)
        {
            clients_con--;
            close(sockfd);
        }
    }
    END:
        close(sockfd);
        pool_cleanup(num_of_clients);
        free(fd_queue);
        fd_queue = NULL;
        free(pool);
        pool = NULL;
        return EXIT_SUCCESS;
}
