/**
 * BBM 342: Operating Systems (Spring 2017)
 * Experiment 2
 * Parallel File Search System
 *
 * Compile: make
 * Note:    I cannot upload Makefile so, you can compile with this command:
 *          gcc main.c -o main -Wall -ansi -lpthread
 *          gcc minion.c -o minion -Wall -ansi -lpthread
 *
 * Run:     ./main <minion_count> <buffer_size> <search_query> <search_path>
 *
 * Tags: fork, exec, pipe, process, pthreads, thread, posix, unix
 * @author Halil Ibrahim Sener <b21328447@cs.hacettepe.edu.tr>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include "main.h"

#define READ_END 0
#define WRITE_END 1

/**
 * Main function
 * Creates the file buffer.
 * Creates minion processes and their controller threads.
 * Creates the searcher thread.
 * @param argc argument count
 * @param argv argument vector
 * @return status value as integer
 */
int main(int argc, char *argv[]) {
    if (argc < 5) {
        printf("Usage: %s <minion_count> <buffer_size> <search_query> <search_path>\n", argv[0]);
        return EXIT_FAILURE;
    }
    int i;
    int minion_count = atoi(argv[1]);
    pthread_t *controller_threads = (pthread_t *) malloc(minion_count * sizeof(pthread_t));

    /* create buffer */
    buffer_t* buffer = create_buffer(atoi(argv[2]));

    /* create search logs and its mutex */
    FILE* logs = fopen("searchlog.txt", "w");
    fprintf(logs, "Log File\nSearch for \"%s\" string\n", argv[3]);
    fprintf(logs, "----------------------\n");

    sem_t *logs_mutex = (sem_t *) malloc(sizeof(sem_t));
    sem_init(logs_mutex, 0, 1);

    for (i = 0; i < minion_count; ++i) {
        /* create pipes */
        int parent_pipefd[2], child_pipefd[2];
        if (pipe(parent_pipefd) < 0 || pipe(child_pipefd)) {
            fprintf(stderr, "[create-pipes] pipe failed.\n");
            exit(EXIT_FAILURE);
        }

        /* fork process */
        pid_t pid = fork();
        if (pid < (pid_t) 0) {
            fprintf(stderr, "[fork-process] fork failed.\n");
            return EXIT_FAILURE;
        }

        /* minion process */
        if (pid == (pid_t) 0) {
            /* change its stdin and stdout */
            dup2(child_pipefd[READ_END], STDIN_FILENO);
            close(child_pipefd[READ_END]);

            dup2(parent_pipefd[WRITE_END], STDOUT_FILENO);
            close(parent_pipefd[WRITE_END]);

            char id[5];
            sprintf(id, "%d", i + 1);
            /* pass its id and search query */
            char *minion_argv[4] = { "./minion", id, argv[3], NULL };
            if (execvp(minion_argv[0], minion_argv) < 0) {
                fprintf(stderr, "[minion-process] execvp failed.\n");
                exit(EXIT_FAILURE);
            }
        }

        /* create control thread */
        controller_args_t *controller_args = malloc(sizeof(controller_args_t));
        controller_args->buffer = buffer;
        controller_args->read_end = parent_pipefd[READ_END];
        controller_args->write_end = child_pipefd[WRITE_END];
        controller_args->logs = logs;
        controller_args->logs_mutex = logs_mutex;
        pthread_create(controller_threads + i, NULL, controller_routine, (void *) controller_args);
    }

    /* main process */

    /* create searcher thread */
    pthread_t searcher_thread;
    searcher_args_t *searcher_args = malloc(sizeof(searcher_args_t));
    searcher_args->buffer = buffer;
    searcher_args->path = argv[4];
    searcher_args->minion_count = minion_count;
    pthread_create(&searcher_thread, NULL, searcher_routine, (void *) searcher_args);

    /* join threads */
    pthread_join(searcher_thread, NULL);
    for (i = 0; i < minion_count; ++i) {
        pthread_join(controller_threads[i], NULL);
    }

    free(searcher_args);
    free(logs_mutex);
    destroy_buffer(buffer);
    free(controller_threads);
    fclose(logs);

    return EXIT_SUCCESS;
}

/**
 * Subroutine for the searcher thread.
 * It iterates over the given path and searches txt files in it.
 * It puts found txt files to the buffer.
 * @param args pointer of an searcher_args_t. Please see main.h
 * @return
 */
void* searcher_routine(void* args) {
    searcher_args_t *searcher_args = (searcher_args_t *) args;
    search_for_txt(searcher_args->path, searcher_args->buffer);

    /* put NULLs (as many as number of minions) to inform controller threads. */
    int i;
    for (i = 0; i < searcher_args->minion_count; ++i) {
        put_buffer(searcher_args->buffer, NULL);
    }
    return NULL;
}

/**
 * Searches txt files in a path and puts them in a buffer.
 * @param path a path of a directory as a string
 * @param buffer pointer of an buffer_t
 */
void search_for_txt(char *path, buffer_t *buffer) {
    DIR *dir;
    struct dirent *dp;

    if ((dir = opendir(path)) == NULL) {
        fprintf(stderr, "[searcher-thread] Cannot open path.\n");
        exit(1);
    }

    while ((dp = readdir(dir)) != NULL) {
        char *subpath = (char *) malloc(strlen(path) + strlen(dp->d_name) + 2);
        strcpy(subpath, path);
        strcat(subpath, "/");
        strcat(subpath, dp->d_name);

        /* some filesystems do not provide d_type */
        if (dp->d_type == 0) {
            struct stat path_stat;
            stat(subpath, &path_stat);
            if (S_ISDIR(path_stat.st_mode)) {
                dp->d_type = 4;
            } else if (S_ISREG(path_stat.st_mode)) {
                dp->d_type = 8;
            }
        }

        /* 4 => directory & 8 => regular file */
        if (dp->d_type == 4 && strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) {
            /* iterate over sub directory */
            search_for_txt(subpath, buffer);
        } else if (dp->d_type == 8 && is_txt(dp->d_name)) {
            /* put file to the buffer */
            put_buffer(buffer, subpath);
        }
    }
}

/**
 * Checks if a filename for a txt file.
 * @param name file name as a string
 * @return 1 if string ends with '.txt', otherwise 0;
 */
int is_txt(char *name) {
    size_t length = strlen(name);
    return length > 4 && strcmp(name + length - 4, ".txt") == 0;
}

/**
 * Subroutine for controller threads.
 * It communicates with a minion process via a pipe. It takes a file path
 * from the buffer and sends it to the minion process. It writes results
 * that coming from the minion to the log file.
 * @param args pointer of an controller_args_t. Please see main.h
 * @return NULL
 */
void* controller_routine(void* args) {
    controller_args_t *controller_args = (controller_args_t *) args;

    while (1) {
        char *file_name = read_buffer(controller_args->buffer);
        if (file_name == NULL) {
            size_t exit = 0;
            write(controller_args->write_end, &exit, sizeof(exit));
            return NULL;
        }

        size_t file_size = strlen(file_name) + 1;
        write(controller_args->write_end, &file_size, sizeof(file_size));
        write(controller_args->write_end, file_name, strlen(file_name) + 1);
        free(file_name);

        while (1) {
            size_t message_size;
            read(controller_args->read_end, &message_size, sizeof(message_size));
            if (message_size == 0) {
                break;
            }

            char *message = (char *) malloc(message_size);
            read(controller_args->read_end, message, message_size);

            sem_wait(controller_args->logs_mutex);
            fprintf(controller_args->logs, "%s", message);
            sem_post(controller_args->logs_mutex);

            free(message);
        }
    }
}

/**
 * Creates a buffer
 * @param size size of the buffer
 * @return pointer of an buffer_t
 */
buffer_t* create_buffer(int size) {
    buffer_t *buffer = (buffer_t *) malloc(sizeof(buffer_t));

    buffer->array = (char **) malloc(size * sizeof(char *));

    buffer->empty = (sem_t *) malloc(sizeof(sem_t));
    buffer->full = (sem_t *) malloc(sizeof(sem_t));
    buffer->mutex = (sem_t *) malloc(sizeof(sem_t));

    buffer->in = 0;
    buffer->out = 0;
    buffer->size = size;

    /* initialize semaphores */
    sem_init(buffer->empty, 0, (unsigned) size); /* counting semaphore */
    sem_init(buffer->full, 0, 0); /* counting semaphore */
    sem_init(buffer->mutex, 0, 1); /* binary semaphore */

    return buffer;
}

/**
 * Puts the specified item to the buffer.
 * @param buffer pointer of an buffer_t
 * @param value item to be put to the buffer
 */
void put_buffer(buffer_t *buffer, char *value) {
    /*
     * Try to get locks and then put the item. After that, release the locks.
     */
    sem_wait(buffer->empty);
    sem_wait(buffer->mutex);

    buffer->array[buffer->in] = value;
    buffer->in = (buffer->in + 1) % buffer->size;

    sem_post(buffer->mutex);
    sem_post(buffer->full);
}

/**
 * Reads an item from the buffer.
 * @param buffer pointer of an buffer_t
 * @return a string from the buffer
 */
char *read_buffer(buffer_t *buffer) {
    /*
     * Try to get locks and then read an item. Release the locks and
     * return the item.
     */
    sem_wait(buffer->full);
    sem_wait(buffer->mutex);

    char *item = buffer->array[buffer->out];
    buffer->out = (buffer->out + 1) % buffer->size;

    sem_post(buffer->mutex);
    sem_post(buffer->empty);

    return item;
}

/**
 * Deallocate dynamic parameters of the buffer and then deallocate itself.
 * @param buffer pointer of an buffer_t
 */
void destroy_buffer(buffer_t *buffer) {
    free(buffer->array);
    free(buffer->empty);
    free(buffer->full);
    free(buffer->mutex);
    free(buffer);
}
