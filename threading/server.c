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
 * VOID HANDLE_SIGNAL:
 *      PARAMETER: INT SIG_NO - SIGINT value
 *      BRIEF: handle ctrl+c input from user
 *      RETURN: None
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
 * INT GET_FILE_COUNT:
 *      PARAMETER: INT SOCKFD - client socket file descriptor
 *      BRIEF: determines the number of files located within the file server
 *      RETURN: (int) number of DT_REG files in FileServer directory, -1 on error
 */
int get_file_count(int sockfd)
{
    // check if file descriptor is valid
    if (0 > sockfd)
    {
        return -1;
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
    // send number of regular files to client
    bytes_sent = send(sockfd, &file_count, sizeof(int), 0);
    if (0 > bytes_sent)
    {
        printf("Error sending file count\n");
        return -1;
    }
    return file_count;
} // end get file count

/**
 * VOID LIST_DIR:
 *      PARAMETER: INT SOCKFD - client socket file descriptor
 *      BRIEF: creates a list of DT_REG files to send to client (similar to basic ls cmd)
 *      RETURN: None
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
    // check for allocation error
    if (NULL == file_list)
    {
        errno = ENOMEM;
        perror("Could not allocate memory for file_list");
        return;
    }
    // open directory and check for error
    if ((d = opendir("FileServer/")) == NULL)
    {
        errno = ENOTDIR;
        perror("Could not open directory");
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
            // check for error sending file name length
            if (0 > bytes_sent)
            {
                printf("Error sending file name length\n");
                return;
            }
            // send the file names to the client
            name_sent = send(sockfd, file_list[i], strnlen(file_list[i], MAX_STR_LEN), 0);
            // check for error sending file name
            if (0 > name_sent)
            {
                printf("Error sending directory list\n");
                return;
            }
            // increment iterator
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
 * BOOL IS_FILE:
 *      PARAMETER: CHAR* FILE_NAME - name of file for validity check
 *      BRIEF: determine whether passed file exists within the directory
 *      RETURN: (bool) true/false value whether file exists
 */
bool is_file(char* file_name)
{
    // FileServer directory
    char file_path[] = "FileServer/";
    // allocate memory to create absolute file path
    char* full_path = NULL;
    full_path = calloc(1, (strlen(file_path) + strlen(file_name)) + 1);
    // check for error in memory allocation
    if (NULL == full_path)
    {
        errno = ENOMEM;
        perror("Could not allocate memory for full path");
        return false;
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
 * VOID DOWNLOAD_FILE:
 *      PARAMETER: CHAR* FILE_PASSED - file within server to be sent to client
 *      PARAMETER: INT SOCKFD - client socket file descriptor
 *      BRIEF: function that sents file (in bytes) to client for transfer
 *      RETURN: None
 */
void download_file(char* file_passed, int sockfd)
{
    // local variables
    FILE* fp = NULL;
    char file_path[] = "FileServer/";
    signed int err_code = -1;
    char* buffer = NULL;
    char* full_path = NULL;
    // allocate memory for absolute path
    full_path = calloc(1, (strlen(file_path) + strlen(file_passed)) + 1);
    // check if memory allocation fails
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
    // check if fopen fails
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
        // check if ftell fails
        if (-1 == file_sz)
        {
            perror("ftell error on file");
            free(full_path);
            full_path = NULL;
            return;
        }
        // allocate memory to store file contents into array
        buffer = calloc(file_sz, sizeof(char));
        // check buffer memory allocation fails
        if (NULL == buffer)
        {
            errno = ENOMEM;
            perror("Could not allocate memory for buffer");
            free(full_path);
            full_path = NULL;
            return;
        }
        // rewind file pointer to beginning of file and check for failure
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
            // check for fread error
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

/**
 * VOID UPLOAD_FILE:
 *      PARAMETER: CHAR* FILE_PASSED - file name received from client for upload
 *      PARAMETER: INT FILE_SIZE - size of file to be uploaded
 *      PARAMETER: INT SOCKFD - client socket file descriptor
 *      BRIEF: received file (in bytes) to be transfered to server from client
 *      RETURN: None
 */
void upload_file(char* file_passed, int file_size, int sockfd)
{
    // check for invalid file size is passed
    if (-1 == file_size)
    {
        printf("Error received from client\n");
        printf("Upload failed...\n");
        return;
    }
    // local variables
    FILE* fp = NULL;
    char file_path[] = "FileServer/";
    int bytes_recv = 0;
    signed int err_code = -1;
    char* buffer = NULL;
    char* full_path = NULL;
    // allocate memory for the full_path
    full_path = calloc(1, (strlen(file_path) + strlen(file_passed)) + 1);
    // check for memory allocation failure
    if (NULL == full_path)
    {
        errno = ENOMEM;
        perror("Could not allocate memory for full path");
        return;
    }
    // copy file path into the full path
    strncpy(full_path, file_path, strlen(file_path));
    // concatenate file name passed to the directory path
    strncat(full_path, file_passed, strlen(file_passed));
    // append a null terminator to the file path
    strncat(full_path, "\0", sizeof(char));
    printf("Saving Client File as: %s\n", full_path);
    // check to see if uploaded file already exists on the server
    if (true == is_file(file_passed))
    {
        printf("Overwriting existing file...\n");
        // open and close existing file for overwrite
        fclose(fopen(full_path, "w"));
    }
    // if file does not currently exist on the server
    fp = fopen(full_path, "wb+");
    // check for fopen error
    if (NULL == fp)
    {
        errno = ENOENT;
        perror("Could not open file for writing");
        send(sockfd, &err_code, sizeof(int), 0);
        free(full_path);
        full_path = NULL;
        return;
    }
    // set modifiable variable for file data chunk loop
    long bytes_left = file_size;
    // allocate memory for file data buffer
    buffer = calloc(1, file_size);
    // check for memory allocation failure
    if (NULL == buffer)
    {
        errno = ENOMEM;
        perror("Could not allocate memory for buffer in upload");
        free(full_path);
        full_path = NULL;
    }
    // loop to receive file data from client in chunks
    while (bytes_left > 0)
    {
        bytes_recv = recv(sockfd, buffer, sizeof(char), 0);
        // check for bytes recv failure
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
        // write file data to new file one character at a time
        fprintf(fp, "%c", *buffer);
        // decrement the iterator
        bytes_left -= 1;
    }
    // clean up resources and print success message
    fclose(fp);
    free(full_path);
    full_path = NULL;
    free(buffer);
    buffer = NULL;
    printf("Upload Complete\n");
} // end upload_file

/**
 * VOID END_CONNECTION:
 *      PARAMETER: None
 *      BRIEF: handles the graceful termination of a client connection
 *      RETURN: None
 */
void end_connection()
{
    pthread_mutex_lock(&pool->lock);
    pool->thread_cnt--;
    pthread_mutex_unlock(&pool->lock);
    clients_con--;
    printf("Number of current connections: %ld\n", clients_con);
}

/**
 * VOID* SERVER_FUNC:
 *      PARAMETER: None
 *      BRIEF: handles the server functionality to be passed to each thread
 *      RETURN: (void *) NULL on success, err_code 999 on failure
 */
void* server_func()
{
    // local variables
    int command = 0;
    char client_msg[MAX_STR_LEN] = { 0 };
    char* file_name = NULL;
    int err_code = 999;
    int* p_err = &err_code;
    ssize_t bytes_recv = 0;
    ssize_t msg_recv = 0;
    ssize_t sz_recv = 0;
    int name_len = 0;
    ssize_t file_len_recv = 0;
    ssize_t file_recv = 0;
    int upload_file_sz = 0;
    int fd = 0;
    // initial, infinite server loop
    while (serv_running)
    {
        // lock shared resource
        pthread_mutex_lock(&pool->lock);
        // if server is running but socket fd queue is empty, set threads to idle
        while (serv_running && is_empty(fd_queue))
        {
            pthread_cond_wait(&pool->condition, &pool->lock);
        }
        // if server is no longer running, unlock mutex and return err code
        if (false == serv_running)
        {
            pthread_mutex_unlock(&pool->lock);
            return (void *) p_err;
        }
        // pop client socket fd from queue
        fd = dequeue(fd_queue);
        // increment pool struct thread count for new client
        pool->thread_cnt++;
        // unlock shared resource
        pthread_mutex_unlock(&pool->lock);
        // alert threads that the shared resource is no longer locked
        pthread_cond_broadcast(&pool->condition);
        // begin server data handling
        while (serv_running)
        {
            // receive initial command from client
            bytes_recv = recv(fd, &command, sizeof(int), 0);
            // check for recv errors
            if (0 > bytes_recv)
            {
                errno = EINVAL;
                perror("No message received from client");
                break;
            }
            // display command received
            printf("Received from client: %d\n", command);
            // switch based upon command received
            switch(command)
            {
                // command = 100, send dir list to client
                case(LIST):
                    if (LIST == command)
                    {
                        // receive next step of the list command
                        msg_recv = recv(fd, client_msg, sizeof(client_msg), 0);
                        // print next command to server
                        printf("CLIENT_MSG: %s\n", client_msg);
                        if (0 == (strncmp(client_msg, "ls", strlen(client_msg))))
                        {
                            printf("Sending directory list...\n");
                            list_dir(fd);
                        }
                        else
                        {
                            // should never reach this, but prints error message
                            printf("Invalid list command received\n");
                        }
                    }
                    break;
                // command = 200, prepare file for download
                case(DOWNLOAD):
                    if (DOWNLOAD == command)
                    {
                        // receive next step of download command
                        msg_recv = recv(fd, client_msg, sizeof(client_msg), 0);
                        // check if file to be downloaded exists
                        if (true == is_file(client_msg))
                        {
                            // print success message
                            printf("OK\n");
                            printf("Sending client %s contents ...\n", client_msg);
                            // call download file function
                            download_file(client_msg, fd);
                        }
                        // if file doesnt exist
                        else
                        {
                            // download_file handles invalid file and sends error code
                            download_file(client_msg, fd);
                        }
                    }
                    break;
                // command = 300, prepare server for file upload
                case(UPLOAD):
                    if (UPLOAD == command)
                    {
                        // receive next step of upload command
                        msg_recv = recv(fd, client_msg, sizeof(client_msg), 0);
                        if (0 == (strncmp(client_msg, "upload", strlen(client_msg))))
                        {
                            printf("upload request received\n");
                            // receive size of file to be uploaded
                            sz_recv = recv(fd, &upload_file_sz, sizeof(int), 0);
                            // check for recv error
                            if (0 == sz_recv)
                            {
                                upload_file_sz = -1;
                                command = 666;
                                goto END;
                            }
                            // check for error code from client of invalid file
                            else if (666 == upload_file_sz || 500 == upload_file_sz)
                            {
                                upload_file_sz = -1;
                                goto END;
                            }
                            printf("Uploading file of size %d from client\n", upload_file_sz);
                            // get length of file name to be uploaded
                            file_len_recv = recv(fd, &name_len, sizeof(int), 0);
                            // check for recv error
                            if (0 >= file_len_recv)
                            {
                                errno = EINVAL;
                                perror("Upload file name size not received");
                                break;
                            }
                            // allocate memory for file_name to be uploaded
                            file_name = calloc(1, (name_len + 1));
                            // check for memory allocation failure
                            if (NULL == file_name)
                            {
                                errno = ENOMEM;
                                perror("Could not allocate memory for file_name");
                                break;
                            }
                            // receive the name of the file to be uploaded to the server
                            file_recv = recv(fd, file_name, name_len, 0);
                            // check for recv errors
                            if (0 >= file_recv)
                            {
                                errno = EINVAL;
                                perror("Upload file name not received");
                                break;
                            }
                            printf("File name received: %s\n", file_name);
                            END:
                            // upload file handles both valid receipt and errors
                                upload_file(file_name, upload_file_sz, fd);
                                free(file_name);
                                file_name = NULL;
                        }
                    }
                    break;
                // command = 500, graceful client exit from connection
                case(EXIT):
                    if (EXIT == command)
                    {
                        // receive the next step to the exit command
                        msg_recv = recv(fd, client_msg, sizeof(client_msg), 0);
                        if (0 == (strncmp(client_msg, "exit", strlen(client_msg))))
                        {
                            // set global exiting value to true
                            exiting = true;
                            // set server command
                            command = -1;
                        }
                        printf("Client has ended the connection...\n");
                    }
                    break;
                // if invalid command is received from client
                default:
                    printf("Invalid command received from the client\n");
                    // this means an error occurred and the client will be disconnected
                    command = EXIT;
                    break;
            }
            // zero-ize the client message for future comms
            memset(client_msg, 0, sizeof(client_msg));
            // if server command is error, break the server loop
            if (command == -1)
            {
                break;
            }
        }
        // if errors occur, gracefully end the client connection
		if (0 >= bytes_recv || 0 >= msg_recv || true == exiting)
		{
			end_connection();
		}
    }
    // return null on success
    return NULL;
} // end server_func

/**
 * INT INIT_THREADPOOL:
 *      PARAMETER: THREADPOOL* POOL - pointer to the pool struct
 *      PARAMETER: SIZE_T NUM_OF_CLIENTS - number of current connections
 *      BRIEF: initializes threadpool holding specified number of threads setting
 *              them to an idle state waiting for handoff to the client
 *      RETURN: (int) 0 on success, -1 on error
 */
int init_threadpool(threadpool* pool, size_t num_of_clients)
{
    // check if pool points to a null struct
    if (NULL == pool)
    {
        errno = EINVAL;
        perror("Pool passed is NULL");
        return -1;
    }
    // allocate memory for the thread array
    pool->threads = calloc(num_of_clients, sizeof(pthread_t));
    // check for memory allocation failure
    if (NULL == pool->threads)
    {
        errno = ENOMEM;
        perror("Could not allocate memory for pool->threads");
        free(pool);
        pool = NULL;
        return -1;
    }
    // set struct member max_thread_cnt to the number of clients specified
    pool->max_thread_cnt = num_of_clients;
    // initialize the mutex
    pthread_mutex_init(&pool->lock, NULL);
    // initialize the signal condition
    pthread_cond_init(&pool->condition, NULL);
    // loop through to create all necessary threads for the application
    for (size_t i = 0; i < num_of_clients; i++)
    {
        // check for pthread_create success/failure
        if (0 != pthread_create(&pool->threads[i], NULL, server_func, NULL))
        {
            errno = EINVAL;
            perror("Error creating thread");
        }
    }
    //return 0 on success
    return 0;
}

/**
 * VOID HANDLE_INCOMING_CLIENT:
 *      PARAMETER: INT CLIENT_FD - client socket file descriptor
 *      BRIEF: obtain socket fd from queue and hand it off to thread for
 *              for network connection to server
 *      RETURN: None
 */
void handle_incoming_client(int client_fd)
{
    // item variable
    item* q_item = NULL;
    // allocate memory for item struct
    q_item = calloc(1, sizeof(item));
    // check for memory allocation failure
    if (NULL == q_item)
    {
        errno = ENOMEM;
        perror("Could not allocate memory for q_item");
        exit(-1);
    }
    // set passed client socket fd to data member of item struct
    q_item->data = client_fd;
    // lock the shared resource
    pthread_mutex_lock(&pool->lock);
    // push the client fd into the queue
    enqueue(fd_queue, *q_item);
    // unlock shared resource
    pthread_mutex_unlock(&pool->lock);
    // alert threads that there is a socket fd to grab
    pthread_cond_broadcast(&pool->condition);
    // clean up item memory allocation
    free(q_item);
    q_item = NULL;
}

/**
 * VOID POOL_CLEANUP:
 *      PARAMETER: SIZE_T THREADS - number of actual threads created and connected
 *      BRIEF: joins all active threads and cleans up resources for proper clean up
 *      RETURN: None
 */
void pool_cleanup(size_t threads)
{
    // loop through thread array
    for (size_t i = 0; i < threads; i++)
    {
        // wake up all threads to prevent deadlocks
        pthread_cond_broadcast(&pool->condition);
        // join all threads
        pthread_join(pool->threads[i], NULL);
    }
    // call queue clean up function
    clear(fd_queue);
    // free thread array
    free(pool->threads);
    pool->threads = NULL;
    // destroy the mutex
    pthread_mutex_destroy(&pool->lock);
    // destroy the signal condition
    pthread_cond_destroy(&pool->condition);
    // free the memory for pool struct
    free(pool);
    pool = NULL;
}

/**
 * INT MAIN:
 *      PARAMETER: INT ARGC - number of command line arguments
 *      PARAMETER: CHAR** ARGV - array storing actual command line arguments
 *      BRIEF: main driver function for the server
 *      RETURN: 0 on success, error code on failure
 */
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
    // check for appropriate amount of cmdline arguments
    if (1 >= argc)
    {
        // set errno value and error message
        errno = EINVAL;
        perror("Invalid arguments passed\nRequired Argument\n\t-p [SRV_PORT]:"
				"Optional Argument\n\t-t [MAX_ALLOWED_CLIENTS]\n");
        exit(EXIT_FAILURE);
    }
    // iterate over cmdline arguments
    while ((opt = getopt(argc, argv, ":p:t:")) != -1)
    {
        // switch on arguments passed
        switch(opt)
        {
            // set p to be port number argument
            case 'p':
                // cast from cmdline string to long int
                port = strtol(optarg, NULL, 10);
                // only allow user to set valid port, check for invalid value
                if (1025 > port || 65535 < port)
                {
                    errno = EINVAL;
                    // create error message
                    perror("Invalid port argument passed\n"
                    "Valid port range: 1025 - 65535");
                    exit(EXIT_FAILURE);
                }
                break;
            // set t to be number of client threads allowed
            case 't':
                // cast from cmdline string to long int
                thread_cnt = strtol(optarg, NULL, 10);
                // make sure value is >= 1
                if (1 > thread_cnt)
                {
                    errno = EINVAL;
                    // create error message
                    perror("invalid thread number passed, must be >= 1");
                    exit(EXIT_FAILURE);
                }
                // set number of allowed clients to specified thread count
                num_of_clients = thread_cnt;
                // make sure client doesn't set ridiculous client amount
                if (MAX_CLIENTS < num_of_clients)
                {
                    printf("Server cannot contain more than %d clients\n"
                            "Setting default to: %d\n", MAX_CLIENTS, MAX_CLIENTS);
                    num_of_clients = MAX_CLIENTS;
                }
                // if no -t flag is set, set thread count to MAX_CLIENTS
				else if ('?' == *optarg)
				{
					printf("No client capacity set: setting default to %d\n", MAX_CLIENTS);
					num_of_clients = MAX_CLIENTS;
				}
                break;
            // if no arguments are set, display appropriate error message
            default:
                printf("Invalid cmdline argument passed\nRequired Argument\n\t-p [SRV_PORT]:\n"
                        "Optional Argument\n\t-t [CLIENT_AMOUNT] (argument optional)\n");
                exit(-1);
        }
    }
    // call signal handler function
    struct sigaction ig_SIGINT = { 0 };
    ig_SIGINT.sa_handler = handle_signal;
    // check for sigaction failure
    if (0 > sigaction(SIGINT, &ig_SIGINT, NULL))
    {
        exit(-1);
    }
    // set up sockaddr struct
	memset(&cli_addr, 0, sizeof(cli_addr));
    // set addr = GLOBAL address
	char* addr = SERV_ADDR;
    // initialize client socket fd queue
    fd_queue = init();
    // allocate memory for threadpool struct
    pool = calloc(1, sizeof(threadpool));
    // check for memory allocate failure
    if (NULL == pool)
    {
        errno = ENOMEM;
        perror("Could not allocat memory for pool in main");
        exit(EXIT_FAILURE);
    }
    // initialize threadpool
    init_threadpool(pool, num_of_clients);
    // create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    // check for socket failure
    if (-1 == sockfd)
    {
        errno = ENOTSOCK;
        perror("Could not create socket");
        exit(EXIT_FAILURE);
    }
    // set sockopt for reuse, check for failure
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1)
    {
        errno = ENOTSOCK;
        perror("Could not set socket option");
        exit(EXIT_FAILURE);
    }
    // set up server address sockaddr struct
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
    // set server address
	serv_addr.sin_addr.s_addr = inet_addr(addr);
    // set socket port
	serv_addr.sin_port = htons(port);
    // call socket bind
    if ((bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) != 0)
    {
        perror("Could not bind on socket");
        exit(EXIT_FAILURE);
    }
    // listen for incoming connections, check for failure
    if ((listen(sockfd, 1)) != 0)
    {
        perror("listen call failed");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Server listening on - %s:%d\n", addr, port);
    }
    // set client length = sizeof sockaddr struct
    cli_addr_len = sizeof(cli_addr);
    // start infinite server loop
    while (serv_running)
    {
        // display total number of clients allowed and number currently connected
        printf("Number of total clients allowed: %ld\n", num_of_clients);
        printf("Number of current connections to server: %ld\n", clients_con);
        // accept incoming connections
        int connfd = accept(sockfd, (struct sockaddr *)&cli_addr, &cli_addr_len);
        // increment number of clients connected upon successful connection
        clients_con++;
        // check for failure
        if (0 > connfd)
        {
            printf("Cleaning up resources\n");
            goto END;
        }
        // call handle clients function
        handle_incoming_client(connfd);
        // check if global connection running variable changed
        if (false == serv_running)
        {
            // call cleanup function, break out of the loop
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
        pool_cleanup(num_of_clients);
        free(fd_queue);
        fd_queue = NULL;
        free(pool);
        pool = NULL;
        return EXIT_SUCCESS;
}
