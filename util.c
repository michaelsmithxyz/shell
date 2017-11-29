#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

#define BUFFER_ALLOC_CAPACITY 10


struct string_part {
    char *content; // String content (not null-terminated)
    size_t length;
};
typedef struct string_part string_part;

struct string_buffer {
    size_t size;
    size_t capacity;
    string_part **parts;
};


string_buffer *init_string_buffer(void) {
    string_buffer *buffer = malloc(sizeof(string_buffer));
    buffer->capacity = BUFFER_ALLOC_CAPACITY;
    buffer->parts = malloc(sizeof(string_part *) * buffer->capacity);
    buffer->size = 0;
    return buffer;
}

void free_string_buffer(string_buffer *buffer) {
    for (int i = 0; i < buffer->size; i++) {
        free(buffer->parts[i]->content);
        free(buffer->parts[i]);
    }
    free(buffer->parts);
    free(buffer);
}

static string_part *make_part(char *content) {
    string_part *part = malloc(sizeof(string_part));
    part->length = strlen(content);
    part->content = malloc(sizeof(char) * part->length);
    memcpy(part->content, content, sizeof(char) * part->length);
    return part;
}

static void append_part(string_buffer *buffer, string_part *part) {
    if (buffer->size >= buffer->capacity) {
        buffer->capacity = buffer->capacity * 2;
        buffer->parts = realloc(buffer->parts, sizeof(string_part *) * buffer->capacity);
    }

    buffer->parts[buffer->size] = part;
    buffer->size++;
}

void push_string(string_buffer *buffer, char *str) {
    string_part *part = make_part(str);
    append_part(buffer, part);
}

size_t string_buffer_size(string_buffer *buffer) {
    size_t size = 0;
    for (int i = 0; i < buffer->size; i++) {
        size += buffer->parts[i]->length;
    }
    return size;
}

char *build_string(string_buffer *buffer) {
    char *buf = malloc(sizeof(char) * string_buffer_size(buffer) + 1);
    size_t offset = 0;
    for (int i = 0; i < buffer->size; i++) {
        memcpy(buf + offset, buffer->parts[i]->content, buffer->parts[i]->length);
        offset += buffer->parts[i]->length;
    }
    buf[offset] = 0;
    return buf;
}
