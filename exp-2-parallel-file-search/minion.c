#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "minion.h"

/**
 * Main function of minion
 * Reads a file from a controller thread of the main process. And searches
 * the query in this file.
 * @param argc argument count
 * @param argv argument vector
 * @return status value as integer
 */
int main(int argc, char *argv[]) {
    char output_file[15];
    sprintf(output_file, "minion%s.out", argv[1]);
    FILE* out = fopen(output_file, "w");

    fprintf(out, "Search for \"%s\" string on Minion%s\n", argv[2], argv[1]);
    fprintf(out, "----------------------\n");

    while (1) {
        size_t input_size;
        read(STDIN_FILENO, &input_size, sizeof(input_size));

        if (input_size == 0) {
            fclose(out);
            return EXIT_SUCCESS;
        }

        char *input = (char *) malloc(input_size);
        read(STDIN_FILENO, input, input_size);
        search_in_file(out, argv[1], argv[2], input);
    }
}

/**
 * Searches the query line by line in a file.
 * @param out output file descriptor
 * @param id minion process' id
 * @param query search query
 * @param file input file
 */
void search_in_file(FILE *out, char *id, char *query, char *file) {
    FILE* in = fopen(file, "r");
    if (in == NULL) {
        fprintf(stderr, "Cannot open the file.\n");
        return;
    }
    to_lower_case(query);

    char *line = NULL;
    size_t len = 0;

    int line_number = 1;
    while (getline(&line, &len, in) != -1) {
        to_lower_case(line);

        char *match = line;
        int offset = 0;
        while ((match = strstr(match, query)) != NULL) {
            unsigned long pos = match - line + 1;
            char message[256];
            sprintf(message, "minion%s: %s:%d:%ld\n", id, file, line_number, pos + offset);

            size_t message_size = strlen(message) + 1;
            fprintf(out, "%s", message);
            write(STDOUT_FILENO, &message_size, sizeof(message_size));
            write(STDOUT_FILENO, message, message_size);

            match++;
        }
        line_number++;
    }
    size_t message_size = 0;
    write(STDOUT_FILENO, &message_size, sizeof(message_size));

    fclose(in);
}

/**
 * Converts all of the characters in a string to lower case.
 * @param str char pointer of a string
 */
void to_lower_case(char *str) {
    int i;
    for (i = 0; str[i]; i++) {
        str[i] = (char) tolower(str[i]);
    }
}
