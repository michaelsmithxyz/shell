#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tokens.h"


struct lexer_context {
    char *input_buffer;
    char *position;
};

struct lexer_token {
    token_type type;
    char *value;
};

#define TOKEN_LIST_INITIAL_CAPACITY 256

struct lexer_token_list {
    lexer_token **contents;
    size_t capacity;
    size_t size;
};


lexer_context *init_lexer(char *input) {
    char *input_buffer = malloc(sizeof(char) * (strlen(input) + 1));
    strcpy(input_buffer, input);

    lexer_context *context = malloc(sizeof(lexer_context));
    context->input_buffer = input_buffer;
    context->position = context->input_buffer;

    return context;
}

void free_lexer(lexer_context *lexer) {
    free(lexer->input_buffer);
    // lexer->position should point to the same allocation of memory

    free(lexer);
}


static lexer_token *build_token(token_type type, char *value) {
    char *token_value = malloc(sizeof(char) * (strlen(value) + 1));
    lexer_token *token = malloc(sizeof(lexer_token));
    strcpy(token_value, value);

    token->type = type;
    token->value = token_value;
    return token;
}

char *get_token_value(lexer_token *token) {
    return token->value;
}

token_type get_token_type(lexer_token *token) {
    return token->type;
}

void free_token(lexer_token *token) {
    free(token->value);
    free(token);
}

static char peek(lexer_context *context) {
    return *(context->position);
}

static char accept(lexer_context *context) {
    char val = *(context->position);
    if (val != 0) context->position++;
    return val;
}

static bool is_word_char(char c) {
    return (c != 0) && (isalnum(c) || (strchr(".,/!@#$%^*-_+=~", c) != NULL));
}

static lexer_token *make_quoteword(lexer_context *context) {
    char word_buffer[4096];
    int i = 0;
    accept(context);
    while (1) {
        char next = peek(context);
        if (next == CHAR_ESCAPE) {
            // Escape sequences
            accept(context);
            next = peek(context);
            switch (next) {
                case CHAR_QUOTE:
                case CHAR_ESCAPE:
                    word_buffer[i++] = accept(context);
                    break;
                default:
                    return build_token(TOKEN_ERROR, "Invalid string escape sequence");
            }
        } else if (next == CHAR_QUOTE) {
            accept(context);
            break;
        } else {
            word_buffer[i++] = accept(context);
        }
    }
    word_buffer[i] = '\0';
    return build_token(TOKEN_WORD, word_buffer);
}

static lexer_token *make_word(lexer_context *context) {
    char word_buffer[4096];
    int i = 0;
    char current = peek(context);
    if (!is_word_char(current) && (current != CHAR_ESCAPE)) {
        return build_token(TOKEN_ERROR, "Bad state reading word token");
    }
    while (is_word_char(current) || current == CHAR_ESCAPE) {
        if (current == CHAR_ESCAPE) {
            accept(context);
            char next = peek(context);
            if (next == CHAR_NEWLINE) {
                // Eat newlines
                accept(context);
            } else {
                word_buffer[i++] = accept(context);
            }
        } else {
            word_buffer[i++] = current;
            accept(context);
        }
        current = peek(context);
    }
    word_buffer[i] = '\0';
    return build_token(TOKEN_WORD, word_buffer);
}

lexer_token *next_token(lexer_context *context) {
    char current = peek(context);
    int i = 0;
    char word_buffer[4096];

    if (current == 0) {
        return NULL;
    }
    switch (current) {
        case CHAR_SPACE:
            accept(context);
            return next_token(context);
        case CHAR_NEWLINE:
            accept(context);
            return build_token(TOKEN_NEWLINE, "<newline>");
        case CHAR_TAB:
            accept(context);
            return next_token(context);
        case CHAR_IN:
            accept(context);
            return build_token(TOKEN_REDIR_IN, "<");
        case CHAR_OUT:
            accept(context);
            return build_token(TOKEN_REDIR_OUT, ">");
        case CHAR_ENDSTMT:
            accept(context);
            return build_token(TOKEN_END_EXPR, ";");
        case CHAR_OR:
            accept(context);
            if (peek(context) == CHAR_OR) {
                accept(context);
                return build_token(TOKEN_OR, "||");
            }
            return build_token(TOKEN_PIPE, "|");
        case CHAR_AND:
            accept(context);
            if (peek(context) == CHAR_AND) {
                accept(context);
                return build_token(TOKEN_AND, "&&");
            }
            return build_token(TOKEN_BACKGROUND, "&");
        case CHAR_SUBSHELL_OPEN:
            accept(context);
            return build_token(TOKEN_SUBSHELL_OPEN, "(");
        case CHAR_SUBSHELL_CLOSE:
            accept(context);
            return build_token(TOKEN_SUBSHELL_CLOSE, ")");
        case CHAR_QUOTE:
            return make_quoteword(context);
        case CHAR_ESCAPE:
            accept(context);
            if (peek(context) == CHAR_NEWLINE) {
                accept(context);
                return next_token(context);
            }
        default: ; 
            return make_word(context);
    }
}


lexer_token_list *init_token_list(void) {
    lexer_token_list *list = malloc(sizeof(lexer_token_list));
    list->contents = malloc(sizeof(lexer_token *) * TOKEN_LIST_INITIAL_CAPACITY);
    list->size = 0;
    list->capacity = TOKEN_LIST_INITIAL_CAPACITY;
    return list;
}

void restore_tokens(lexer_token_list *dest, lexer_token_list *src) {
    dest->capacity = src->capacity;
    dest->size = src->size;
    dest->contents = realloc(dest->contents, sizeof(lexer_token *) * dest->capacity);
    for (size_t i = 0; i < dest->size; i++) {
        dest->contents[i] = src->contents[i];
    }
}

// Frees the list itself and every token in it. Should be called on the parent token
// list from the lexer, not by any sublists in the parser
void free_token_list(lexer_token_list *list) {
    for (size_t i = 0; i < list->size; i++) {
        free_token(list->contents[i]);
    }
    free_token_list_container(list);
}

void free_token_list_container(lexer_token_list *list) {
    free(list->contents);
    free(list);
}

lexer_token_list *copy_token_list(lexer_token_list *list) {
    lexer_token_list *new = malloc(sizeof(lexer_token_list));
    new->size = list->size;
    new->capacity = list->capacity;
    new->contents = malloc(sizeof(lexer_token *) * new->capacity);
    for (size_t i = 0; i < new->size; i++) {
        new->contents[i] = list->contents[i];
    }
    return new;
}

void add_token(lexer_token_list *list, lexer_token *token) {
    if (list->size >= list->capacity) {
        list->capacity = list->capacity * 2;
        list->contents = realloc(list->contents, sizeof(lexer_token *) * list->capacity);
    }
    list->contents[list->size++] = token;
}

lexer_token *peek_token(lexer_token_list *list) {
    if (list->size > 0) {
        return list->contents[0];
    }
    return NULL;
}

lexer_token *consume_token(lexer_token_list *list) {
    if (list->size > 0) {
        lexer_token *ret = list->contents[0];
        for (size_t i = 0; i < list->size - 1; i++) {
            list->contents[i] = list->contents[i + 1];
        }
        list->size--;
        return ret;
    }
    return NULL;
}

bool token_list_empty(lexer_token_list *list) {
    return list->size == 0;
}

static void print_token(lexer_token *token) {
    printf("%s\n", get_token_value(token));
}

void print_token_list(lexer_token_list *list) {
    for (size_t i = 0; i < list->size; i++) {
        print_token(list->contents[i]);
    }
}
