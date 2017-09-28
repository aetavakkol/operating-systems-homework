/**
 * BBM 342 Operating Systems
 * Experiment 1
 * Processes and Pipes
 *
 * Compile: make
 *
 * Run:     ./process <process_count>
 * Note:    Also needs text file named "matrix.txt". Please see INPUT_FILE macro
 *
 * Tags: process, pipe, unix
 * @author Halil Ibrahim Sener <b21328447@cs.hacettepe.edu.tr>
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#define INPUT_FILE  ((const char *) "matrix.txt")

/* Function prototypes */
void main_work(int** pipefd, int pcount);
void child_work(int** pipefd, int pnum, int pcount);

int** create_pipefd(int pcount);
void close_pipefd(int** pipefd, int pnum, int pcount);
void free_pipefd(int** pipefd, int pcount);

long long** square_matrix_mult(long long** matrix, int n);
long long** scan_matrix(FILE* matrixFile, int n);
void print_matrix(long long** matrix, int n, int pnum);
int max_length_in_matrix(long long** matrix, int n);
int digit_count(long long number);

void malloc_2d_long(long long*** array, int m, int n);
void free_2d_long(long long*** array);

/**
 * Main function
 * Create pipes and forks childs according to the argument
 * @param argc  argument count
 * @param argv  argument vector
 * @return status value as integer
 * @see main_work
 * @see child_work
 */
int main (int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <process_count>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int pcount = atoi(argv[1]) + 1;
    int** pipefd = create_pipefd(pcount);

    int i;
    for (i = 1; i < pcount; ++i) {
        pid_t pid = fork();
        if (pid < (pid_t) 0) {
            fprintf(stderr, "Create child process(%d) failed.\n", i);
            return EXIT_FAILURE;
        }

        /* Child process */
        if (pid == (pid_t) 0) {
            child_work(pipefd, i, pcount);
            return EXIT_SUCCESS;
        }
    }

    /* Main process */
    main_work(pipefd, pcount);
    return EXIT_SUCCESS;
}

/**
 * Main process work
 * Reads input matrix from the text file and writes it to the pipe.
 * Also reads result matrix of n-th process but nothing with that.
 * @param pipefd    pipe file descriptors
 * @param pcount    total process count
 */
void main_work(int** pipefd, int pcount) {
    close_pipefd(pipefd, 0, pcount); /* close unnecessary pipes */

    FILE* matrixFile = fopen(INPUT_FILE, "r"); /* open matrix.txt */
    int n;
    fscanf(matrixFile, "%d", &n); /* read square matrix size */
    long long** matrix = scan_matrix(matrixFile, n); /* scan matrix */
    fclose(matrixFile); /* close matrix.txt */

    write(pipefd[0][1], &n, sizeof(n)); /* write matrix size */
    write(pipefd[0][1], &matrix[0][0], sizeof(long long) * n * n); /* write matrix */
    close(pipefd[0][1]); /* close write pipe */

    read(pipefd[pcount - 1][0], &n, sizeof(n)); /* read square matrix size from last child process */
    read(pipefd[pcount - 1][0], &matrix[0][0], sizeof(long long) * n * n); /* read matrix */

    close(pipefd[pcount - 1][0]); /* close read pipe */

    free_2d_long(&matrix);
    free_pipefd(pipefd, pcount);
}

/**
 * Child process work
 * Reads matrix from the previous process and calculate square of it. And
 * writes the result to the pipe for next process.
 * @param pipefd    pipe file descriptors
 * @param pnum      process number. 0(main process), 1, 2 and so on
 * @param pcount    total process count
 */
void child_work(int** pipefd, int pnum, int pcount) {
    close_pipefd(pipefd, pnum, pcount);

    int n;
    long long** matrix;

    read(pipefd[pnum - 1][0], &n, sizeof(n)); /* read square matrix size */
    malloc_2d_long(&matrix, n, n); /* allocate contiguous matrix */
    read(pipefd[pnum-1][0], &matrix[0][0], sizeof(long long) * n * n); /* read matrix */

    long long** result = square_matrix_mult(matrix, n); /* calculate square of the matrix */
    print_matrix(result, n, pnum); /* print the matrix to output and a text file */

    close(pipefd[pnum - 1][0]); /* close read pipe */

    write(pipefd[pnum][1], &n, sizeof(n)); /* write matrix size */
    write(pipefd[pnum][1], &result[0][0], sizeof(long long) * n * n); /* write matrix */
    close(pipefd[pnum][1]); /* close write pipe */

    free_2d_long(&matrix);
    free_2d_long(&result);
    free_pipefd(pipefd, pcount);
}

/**
 * Create pipe file descriptors
 * @param pcount total process count
 * @return address of the pipefd
 */
int** create_pipefd(int pcount) {
    int** pipefd = (int**) malloc(sizeof(int*) * pcount);
    int i;
    for (i = 0; i < pcount; ++i) {
        pipefd[i] = (int*) malloc(sizeof(int) * 2);
        if (pipe(pipefd[i])) {
            fprintf(stderr, "Create pipe failed.\n");
            exit(EXIT_FAILURE);
        }
    }
    return pipefd;
}

/**
 * Closes unnecessary pipe file descriptors
 * @param pipefd    pipe file descriptors
 * @param pnum      process number. 0(main process), 1, 2 and so on
 * @param pcount    total process count
 */
void close_pipefd(int** pipefd, int pnum, int pcount) {
    int i;
    for (i = 0; i < pcount; ++i) {
        if ((pnum == 0 && pcount - 1 != i) || (pnum != 0 && pnum != i + 1)) {
            close(pipefd[i][0]);
        }
        if (pnum != i) {
            close(pipefd[i][1]);
        }
    }
}

/**
 * Deallocate pipefd
 * @param pipefd    pipe file descriptors
 * @param pcount    total process count
 * @see create_pipefd
 */
void free_pipefd(int** pipefd, int pcount) {
    int i;
    for (i = 0; i < pcount; ++i) {
        free(pipefd[i]);
    }
    free(pipefd);
}

/**
 * Square matrix multiplication
 * Note: Used an n^3 complexity algorithm. There are better algorithms but
 * the objective of this experiment is to learn and apply process and pipe
 * basics.
 * @param matrix    2D long long array
 * @param n         matrix size
 * @return result matrix address
 */
long long** square_matrix_mult(long long** matrix, int n) {
    long long** result;
    malloc_2d_long(&result, n, n);

    int i, j, k;
    for (i = 0; i < n; ++i) {
        for (j = 0; j < n; ++j) {
            result[i][j] = 0;
            for (k = 0; k < n; ++k) {
                result[i][j] += matrix[i][k] * matrix[k][j];
            }
        }
    }

    return result;
}

/**
 * Scans the matrix from a file descriptor
 * @param matrixFile    file descriptor
 * @param n             size of the matrix
 * @return the matrix address
 */
long long** scan_matrix(FILE* matrixFile, int n) {
    long long** matrix;
    malloc_2d_long(&matrix, n, n);

    int i, j;
    for (i = 0; i < n; ++i) {
        for (j = 0; j < n; ++j) {
            fscanf(matrixFile, "%lld,", &matrix[i][j]);
        }
    }

    return matrix;
}

/**
 * Prints the square matrix to the console and a text file named "$pnum.txt"
 * @param matrix    the matrix address
 * @param n         size of the matrix
 * @param pnum      process number
 */
void print_matrix(long long** matrix, int n, int pnum) {
    int max_length = max_length_in_matrix(matrix, n);
    char filename[7]; /* max 2 digit(10) for filename and 4 for extension(.txt) */
    sprintf(filename, "%d.txt", pnum);
    FILE* out = fopen(filename, "w");

    printf("Process-%d %d\n\n", pnum, getpid());
    fprintf(out, "Process-%d %d\n", pnum, getpid());

    int i, j;
    for (i = 0; i < n; ++i) {
        for (j = 0; j < n; ++j) {
            printf("%-*lld", max_length, matrix[i][j]);
            fprintf(out, "%-*lld", max_length, matrix[i][j]);
        }
        printf("\n");
        fprintf(out, "\n");
    }
    printf("\n");

    fclose(out);
}

/**
 * Returns the maximum number length in a matrix
 * @param matrix    the matrix address
 * @param n         size of the matrix
 * @return the maximum number length
 */
int max_length_in_matrix(long long** matrix, int n) {
    long long max = 0;

    int i, j;
    for (i = 0; i < n; ++i) {
        for (j = 0; j < n; ++j) {
            if (matrix[i][j] > max) {
                max = matrix[i][j];
            } else if (-matrix[i][j] > max) {
                max = -matrix[i][j];
            }
        }
    }
    return digit_count(max) + 2; /* 1 for minus sign and 1 for space */
}

/**
 * Digit count of a number
 * @param number a long long number
 * @return digit count of the number
 */
int digit_count(long long number) {
    char snum[100];
    return sprintf(snum, "%lld", number);
}

/**
 * Contiguous 2D long long array allocation using malloc.
 * @param array destination 2d array pointer
 * @param m     row count
 * @param n     column count
 */
void malloc_2d_long(long long*** array, int m, int n) {

    /* allocate m * n contiguous long long */
    long long* p = (long long*) malloc(m * n * sizeof(long long));
    if (!p) {
        exit(EXIT_FAILURE);
    }

    /* allocate rows */
    *array = (long long**) malloc(m * sizeof(long long*));
    if (!(*array)) {
        free(p);
        exit(EXIT_FAILURE);
    }

    int i;
    for (i = 0; i < m; ++i) {
        (*array)[i] = &(p[i * n]);
    }
}


/**
 * Deallocate contiguous 2D long long array
 * @param array destination 2d array pointer
 * @see malloc_2d_long long
 */
void free_2d_long(long long*** array) {
    free(&((*array)[0][0]));
    free(*array);
}
