#pragma once

#include <stdbool.h>

// Operators and shell tokens
#define CHAR_AND '&'
#define CHAR_OR '|'
#define CHAR_OUT '>'
#define CHAR_IN '<'
#define CHAR_ENDSTMT ';'

#define CHAR_SUBSHELL_OPEN '('
#define CHAR_SUBSHELL_CLOSE ')'

// Input tokens
#define CHAR_ESCAPE '\\'
#define CHAR_QUOTE '"'
#define CHAR_NEWLINE '\n'
#define CHAR_TAB '\t'
#define CHAR_SPACE ' '


struct lexer_context;
typedef struct lexer_context lexer_context;

struct lexer_token;
typedef struct lexer_token lexer_token;

struct lexer_token_list;
typedef struct lexer_token_list lexer_token_list;

enum token_type {
    TOKEN_WORD,

    TOKEN_AND,
    TOKEN_OR,
    TOKEN_BACKGROUND,
    TOKEN_END_EXPR,
    TOKEN_PIPE,
    TOKEN_REDIR_IN,
    TOKEN_REDIR_OUT,

    TOKEN_SUBSHELL_OPEN,
    TOKEN_SUBSHELL_CLOSE,

    TOKEN_NEWLINE,

    TOKEN_ERROR
};
typedef enum token_type token_type;

// Token operations

lexer_context *init_lexer(char *input);
lexer_token *next_token(lexer_context *context);
void free_lexer(lexer_context *lexer);

char *get_token_value(lexer_token *token);
token_type get_token_type(lexer_token *token);
void free_token(lexer_token *token);

// Token list operations

lexer_token_list *init_token_list(void);
void free_token_list(lexer_token_list *list);
void free_token_list_container(lexer_token_list *list);

lexer_token_list *copy_token_list(lexer_token_list *list);
void restore_tokens(lexer_token_list *dest, lexer_token_list *src);

void add_token(lexer_token_list *list, lexer_token *token);

lexer_token *peek_token(lexer_token_list *list);
lexer_token *consume_token(lexer_token_list *list);

bool token_list_empty(lexer_token_list *list);

void print_token_list(lexer_token_list *list);
