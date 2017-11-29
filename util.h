#pragma once


struct string_buffer;
typedef struct string_buffer string_buffer;


string_buffer *init_string_buffer(void);
void free_string_buffer(string_buffer *buffer);

void push_string(string_buffer *buffer, char *str);
char *build_string(string_buffer *buffer);
