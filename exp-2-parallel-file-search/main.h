#ifndef BBM342_EXP2_MAIN_H
#define BBM342_EXP2_MAIN_H

typedef struct buffer {
    char **array;
    sem_t* mutex;
    sem_t* empty;
    sem_t* full;
    int in;
    int out;
    int size;
} buffer_t;

typedef struct searcher_args {
    buffer_t *buffer;
    char *path;
    int minion_count;
} searcher_args_t;

typedef struct controller_args {
    buffer_t *buffer;
    int read_end;
    int write_end;
    FILE *logs;
    sem_t* logs_mutex;
} controller_args_t;

/* Searcher thread routine and its helper methods */
void* searcher_routine(void* args);
void search_for_txt(char *path, buffer_t *buffer);
int is_txt(char *name);

/* Controller thread routine */
void* controller_routine(void* args);

/* Buffer operations */
buffer_t* create_buffer(int size);
void put_buffer(buffer_t *buffer, char *value);
char *read_buffer(buffer_t *buffer);
void destroy_buffer(buffer_t *buffer);

#endif
