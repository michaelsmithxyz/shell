#pragma once

#include <stdio.h>


struct input_line;
typedef struct input_line input_line;


input_line *get_line(FILE *instream, char *prompt);
void free_input_line(input_line *line);

char *line_content(input_line *line);
bool line_hasmore(input_line *line);
