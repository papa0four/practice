#include "../includes/file_operations.h"

int get_file_count (int sockfd)
{
    if (0 > sockfd)
    {
        return -1;
    }

    ssize_t     bytes_sent = 0;
    int         file_count = 0;
    DIR       * p_dir      = NULL;

    struct dirent * dir;

    p_dir = opendir("FileServer/");
    if (p_dir)
    {
        while (NULL != (dir = readdir(p_dir)))
        {
            if (DT_REG == dir->d_type)
            {
                file_count++;
            }
            else
            {
                continue;
            }
        }
        closedir(p_dir);
    }

    bytes_sent = send(sockfd, &file_count, sizeof(int), 0);
    if (0 > bytes_sent)
    {
        printf("Error sending file count\n");
        return -1;
    }
    return file_count;
}

void list_dir (int sockfd)
{
    ssize_t     bytes_sent  = 0;
    ssize_t     name_sent   = 0;
    DIR       * p_dir       = NULL;
    size_t      name_len    = 0;
    char     ** file_list   = NULL;

    int num_files = get_file_count(sockfd);
    if (-1 == num_files)
    {
        fprintf(stderr, "%s could not retrieve file count on server\n", __func__);
        return;
    }

    printf("number of files on server: %d\n", num_files);
    
    if (0 == num_files)
    {
        num_files = 1;
    }

    if ((p_dir = opendir("FileServer/")) == NULL)
    {
        errno = ENOTDIR;
        perror("Could not open directory");
        CLEAN(file_list);
        return;
    }

    int iter = 0;
    struct dirent * dir;

    while (NULL != (dir = readdir(p_dir)) && num_files > iter)
    {
        // if file type is "regular"
        if (DT_REG == dir->d_type)
        {
            name_len = strnlen(dir->d_name, MAX_STR_LEN);
            bytes_sent = send(sockfd, &name_len, sizeof(size_t), 0);
            if (0 > bytes_sent)
            {
                printf("Error sending file name length\n");
                return;
            }

            name_sent = send(sockfd, dir->d_name, name_len, 0);
            if (0 > name_sent)
            {
                printf("Error sending directory list\n");
                return;
            }

            iter++;
        }
    }
    closedir(p_dir);
}

bool is_file (char * p_filename)
{
    char   file_path[] = "FileServer/";
    char * p_fullpath  = NULL;

    p_fullpath = calloc(((strlen(file_path) + strlen(p_filename)) + 1), sizeof(char));
    if (NULL == p_fullpath)
    {
        errno = ENOMEM;
        perror("Could not allocate memory for full path");
        return false;
    }

    strncpy(p_fullpath, file_path, strlen(file_path));
    strncat(p_fullpath, p_filename, strlen(p_filename));

    if (access(p_fullpath, F_OK) == 0)
    {
        CLEAN(p_fullpath);
        return true;
    }

    CLEAN(p_fullpath);
    return false;
}

void download_file (char * p_file_passed, int sockfd)
{
    FILE* fp = NULL;
    char file_path[] = "FileServer/";
    signed int err_code = -1;
    char * p_buffer = NULL;
    char * p_fullpath = NULL;

    p_fullpath = calloc(1, (strlen(file_path) + strlen(p_file_passed)) + 1);
    if (NULL == p_fullpath)
    {
        errno = ENOMEM;
        perror("Could not allocate memory for full path");
        return;
    }

    strncpy(p_fullpath, file_path, strlen(file_path));
    strncat(p_fullpath, p_file_passed,strlen(p_file_passed));

    fp = fopen(p_fullpath, "rb");
    if (NULL == fp)
    {
        errno = ENOENT;
        perror("Could not open file passed");
        send(sockfd, &err_code, sizeof(int), 0);
        CLEAN(p_fullpath);
        return;
    }

    if (fseek(fp, 0L, SEEK_END) == 0)
    {
        long file_sz = ftell(fp);
        if (-1 == file_sz)
        {
            perror("ftell error on file");
            CLEAN(p_fullpath);
            return;
        }

        p_buffer = calloc(file_sz, sizeof(char));
        if (NULL == p_buffer)
        {
            errno = ENOMEM;
            perror("Could not allocate memory for p_buffer");
            CLEAN(p_fullpath);
            return;
        }

        if (fseek(fp, 0L, SEEK_SET) != 0)
        {
            perror("Could not rewind file");
            CLEAN(p_fullpath);
            CLEAN(p_buffer);
            return;
        }

        long bytes_left = file_sz;
        size_t length = 0;
        printf("Sending file of size %ld to client...\n", file_sz);
        file_sz = htobe64(file_sz);
        send(sockfd, &file_sz, sizeof(long), 0);

        while (bytes_left > 0)
        {
            length = fread(p_buffer, sizeof(char), 1024, fp);
            if (ferror(fp) != 0)
            {
                fprintf(stderr, "Error reading file: %s\n", p_file_passed);
                CLEAN(p_fullpath);
                CLEAN(p_buffer);
                return;
            }

            send(sockfd, p_buffer, length, 0);
            bytes_left -= 1024;
        }      
    }

    fclose(fp);
    CLEAN(p_fullpath);
    CLEAN(p_buffer);
}

void upload_file (char * p_file_passed, int file_size, int sockfd)
{
    if (-1 == file_size)
    {
        fprintf(stderr, "Error received from client\n");
        fprintf(stderr, "Upload failed...\n");
        return;
    }

    FILE* fp = NULL;
    char file_path[] = "FileServer/";
    ssize_t bytes_recv = 0;
    signed int err_code = -1;
    char * p_buffer = NULL;
    char * p_fullpath = NULL;
    p_fullpath = calloc(1, (strlen(file_path) + strlen(p_file_passed)) + 1);
    if (NULL == p_fullpath)
    {
        errno = ENOMEM;
        perror("Could not allocate memory for full path");
        return;
    }

    strncpy(p_fullpath, file_path, strlen(file_path));
    strncat(p_fullpath, p_file_passed, strlen(p_file_passed));
    strncat(p_fullpath, "\0", sizeof(char));
    printf("Saving Client File as: %s\n", p_fullpath);
    if (true == is_file(p_file_passed))
    {
        printf("Overwriting existing file...\n");
        fclose(fopen(p_fullpath, "w"));
    }

    fp = fopen(p_fullpath, "wb+");
    if (NULL == fp)
    {
        errno = ENOENT;
        fprintf(stderr, "%s() - Could not open file for writing: %s\n", __func__, strerror(errno));
        send(sockfd, &err_code, sizeof(int), 0);
        CLEAN(p_fullpath);
        return;
    }

    p_buffer = calloc(file_size, sizeof(char));
    if (NULL == p_buffer)
    {
        errno = ENOMEM;
        perror("Could not allocate memory for p_buffer in upload");
        CLEAN(p_fullpath);
        return;
    }

    long bytes_left = file_size;
    while (bytes_left > 0)
    {
        bytes_recv = recv(sockfd, p_buffer, sizeof(char), 0);
        if (0 >= bytes_recv)
        {
            errno = EINVAL;
            fprintf(stderr, "%s could not receive file from client: %s\n", __func__, strerror(errno));
            CLEAN(p_buffer);
            CLEAN(p_fullpath);
            return;
        }

        fprintf(fp, "%c", *p_buffer);
        bytes_left -= 1;
    }

    fclose(fp);
    CLEAN(p_buffer);
    CLEAN(p_fullpath);
    printf("Upload Complete\n");
}

/*** end file_operations.c ***/
