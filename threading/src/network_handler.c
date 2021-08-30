#include "../includes/network_handler.h"

setup_info_t * handle_setup (int argc, char ** argv)
{
    if ((argc <= 1) || (NULL == argv) || (NULL == *argv))
    {
        // set errno value and error message
        errno = EINVAL;
        perror("Invalid arguments passed\nRequired Argument\n\t-p [SRV_PORT]:"
				"Optional Argument\n\t-t [MAX_ALLOWED_CLIENTS]\n");
        return NULL;
    }

    int    opt          = -1;
    long   port         = -1;
    size_t thread_cnt   = 0;

    setup_info_t * p_setup = calloc(1, sizeof(setup_info_t));
    if (NULL == p_setup)
    {
        errno = ENOMEM;
        fprintf(stderr, "%s could not allocate setup info container: %s\n", __func__, strerror(errno));
        return NULL;
    }

    while ((opt = getopt(argc, argv, ":p:t:")) != -1)
    {
        switch(opt)
        {
            case 'p':
                port = strtol(optarg, NULL, BASE_10);
                if (MIN_PORT > port || MAX_PORT < port)
                {
                    errno = EINVAL;
                    // create error message
                    perror("Invalid port argument passed\n"
                    "Valid port range: 1025 - 65535");
                    return NULL;
                }
                p_setup->port = calloc(MAX_PORT_LEN, sizeof(char));
                if (NULL == p_setup->port)
                {
                    errno = ENOMEM;
                    fprintf(stderr, "%s could not allocate for port member of setup: %s\n", __func__, strerror(errno));
                    return NULL;
                }
                memcpy(p_setup->port, optarg, MAX_PORT_LEN);
                break;

            case 't':
                thread_cnt = strtol(optarg, NULL, 10);
                if (1 > thread_cnt)
                {
                    errno = EINVAL;
                    perror("invalid thread number passed, must be >= 1");
                    exit(EXIT_FAILURE);
                }

                p_setup->num_allowable_clients = thread_cnt;

                if (MAX_CLIENTS < p_setup->num_allowable_clients)
                {
                    printf("Server cannot contain more than %d clients\n"
                            "Setting default to: %d\n", MAX_CLIENTS, MAX_CLIENTS);
                    p_setup->num_allowable_clients = MAX_CLIENTS;
                }
				else if ('?' == *optarg)
				{
					printf("No client capacity set: setting default to %d\n", MAX_CLIENTS);
					p_setup->num_allowable_clients = MAX_CLIENTS;
				}
                break;

            default:
                printf("Invalid cmdline argument passed\nRequired Argument\n\t-p [SRV_PORT]:\n"
                        "Optional Argument\n\t-t [CLIENT_AMOUNT] (argument optional)\n");
                exit(-1);
        }
    }

    return p_setup;
}

struct addrinfo setup_server (struct addrinfo * p_serv)
{
    p_serv->ai_family   = AF_UNSPEC;
    p_serv->ai_socktype = SOCK_STREAM;
    p_serv->ai_flags    = AI_PASSIVE;

    return * p_serv;
}

int setup_socket (struct addrinfo * p_serv, char * p_port)
{
    int sock_fd     = -1;
    int ret_val     = -1;

    struct addrinfo * serv_info;
    struct addrinfo * p_addr;
    
    struct timeval    timeout;
    timeout.tv_sec  = 10;
    timeout.tv_usec = 0;

    if ((ret_val = getaddrinfo(NULL, p_port, p_serv, &serv_info))!= 0)
    {
        fprintf(stderr, "%s getaddrinfo() - %s\n", __func__, gai_strerror(ret_val));
        return -1;
    }

    for (p_addr = serv_info; p_addr != NULL; p_addr = p_addr->ai_next)
    {
        if (-1 == (sock_fd = socket(p_addr->ai_family, p_addr->ai_socktype | SOCK_NONBLOCK, p_addr->ai_protocol)))
        {
            fprintf(stderr, "%s call to socket failed\n", __func__);
            continue;
        }

        if (-1 == setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &timeout, sizeof(timeout)))
        {
            fprintf(stderr, "%s could not set socket options\n", __func__);
            return -1;
        }

        if (-1 == bind(sock_fd, p_addr->ai_addr, p_addr->ai_addrlen))
        {
            close(sock_fd);
            fprintf(stderr, "%s could not bind server socker\n", __func__);
            continue;
        }
        break;
    }

    freeaddrinfo(serv_info);
    return sock_fd;
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

void end_connection()
{
    pthread_mutex_lock(&p_tpool->lock);
    p_tpool->thread_cnt--;
    pthread_mutex_unlock(&p_tpool->lock);
    clients_con--;
    printf("Number of current connections: %ld\n", clients_con);
}