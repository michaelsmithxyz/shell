#pragma once

#include <stdbool.h>

#include "tokens.h"


enum redir_type {
    REDIR_OUT, // >
    REDIR_IN   // <
};
typedef enum redir_type redir_type;

// Capture redirection information
struct redir_info {
    redir_type type;
    char *target_filename;
};
typedef struct redir_info redir_info;

enum parse_tree_type {
    PARSE_TREE_NONE,
    PARSE_TREE_COMMAND,
    PARSE_TREE_AND,         // a && b
    PARSE_TREE_OR,          // a || b
    PARSE_TREE_PIPE,        // a | b
    PARSE_TREE_BACKGROUND,  // a & b
    PARSE_TREE_LIST,        // a ; b  or a \n b
    PARSE_TREE_ERROR
};
typedef enum parse_tree_type parse_tree_type;

// The big parse tree structure to represent sh programs
//
typedef struct parse_tree parse_tree;
struct parse_tree {
    parse_tree_type type;

    size_t argc;
    char *argv[256];

    redir_info *redirections[2];

    // Child parse trees for binary operators
    parse_tree *left;
    parse_tree *right;
};


parse_tree *parse(lexer_token_list *tokens);

void free_parse_tree(parse_tree *tree);
