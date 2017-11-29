#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "input.h"
#include "tokens.h"


#define INPUT_BUFFER_SIZE 1024


struct input_line {
    char *content;
    bool more;
};


static input_line *init_input_line(size_t size) {
    input_line *line = malloc(sizeof(input_line));
    line->more = false;
    line->content = malloc(sizeof(char) * size + 1);
    return line;
}


input_line *get_line(FILE *instream, char *prompt) {
    char in_buffer[INPUT_BUFFER_SIZE];
    
    fprintf(stdout, prompt);
    fflush(stdout);
    
    char *i = fgets(in_buffer, INPUT_BUFFER_SIZE, instream);
    if (i == NULL) {
        return NULL;
    }
    
    size_t input_len = strlen(in_buffer);
    input_line *line = init_input_line(input_len);
    strncpy(line->content, in_buffer, input_len + 1);

    if (input_len <= 0 || in_buffer[input_len - 1] != CHAR_NEWLINE) {
        // Something weird happened
        free_input_line(line);
        return NULL;
    }
    if (input_len >= 2 && (in_buffer[input_len - 2] == CHAR_ESCAPE)) {
        line->more = true;
    } else {
        line->more = false;
    }
    return line;
}

void free_input_line(input_line *line) {
    free(line->content);
    free(line);
}

char *line_content(input_line *line) {
    return line->content;
}

bool line_hasmore(input_line *line) {
    return line->more;
}
