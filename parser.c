#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "tokens.h"



static parse_tree *parse_command(lexer_token_list *tokens);
static parse_tree *parse_pipeline(lexer_token_list *tokens);
static parse_tree *parse_list(lexer_token_list *tokens);

static parse_tree *init_tree(void) {
    parse_tree *tree = malloc(sizeof(parse_tree));
    tree->type = PARSE_TREE_NONE;
    tree->argc = 0;
    for (size_t i = 0; i < 256; i++) {
        tree->argv[i] = NULL;
    }
    tree->redirections[0] = NULL;
    tree->redirections[1] = NULL;
    tree->left = NULL;
    tree->right = NULL;
    return tree;
}

static parse_tree *error_tree(char *message) {
    parse_tree *tree = init_tree();
    tree->type = PARSE_TREE_ERROR;
    tree->argc = 1;
    tree->argv[0] = malloc(sizeof(char) * strlen(message) + 1);
    strcpy(tree->argv[0], message);
    return tree;
}

static parse_tree *parse_command(lexer_token_list *tokens) {
    if (token_list_empty(tokens)) {
        return init_tree();
    }
    
    lexer_token_list *original_tokens = copy_token_list(tokens);
    lexer_token *next = peek_token(tokens);
    if (get_token_type(next) != TOKEN_WORD && get_token_type(next) != TOKEN_SUBSHELL_OPEN) {
        free_token_list_container(original_tokens);
        return init_tree();
    }

    if (get_token_type(next) == TOKEN_SUBSHELL_OPEN) {
        consume_token(tokens); // Eat the (
        parse_tree *subexp = parse_list(tokens);
        next = peek_token(tokens);
        if (!next || get_token_type(next) != TOKEN_SUBSHELL_CLOSE) {
            free_parse_tree(subexp);
            free_token_list_container(original_tokens);
            return error_tree("Expected ) to terminate subexpression");
        }
        consume_token(tokens);
        return subexp;
    }

    // Otherwise, have a word
    parse_tree *tree = init_tree();
    tree->type = PARSE_TREE_COMMAND;

    while (next && get_token_type(next) == TOKEN_WORD) {
        char *value = get_token_value(next);
        tree->argv[tree->argc] = malloc((sizeof(char) * strlen(value)) + 1);
        strcpy(tree->argv[tree->argc], value);
        tree->argc++;

        consume_token(tokens);
        next = peek_token(tokens);
    }

    // First redirection
    if (next && (get_token_type(next) == TOKEN_REDIR_IN || get_token_type(next) == TOKEN_REDIR_OUT)) {
        redir_type redir1_type = get_token_type(next) == TOKEN_REDIR_IN ? REDIR_IN : REDIR_OUT;
        consume_token(tokens);
        next = consume_token(tokens);
        if (get_token_type(next) != TOKEN_WORD) {
            free_parse_tree(tree);
            free_token_list_container(original_tokens);
            return error_tree("Redirection must be followed by a filename");
        }
        redir_info *redir1_info = malloc(sizeof(redir_info));
        redir1_info->type = redir1_type;
        char *redir1_value = get_token_value(next);
        redir1_info->target_filename = malloc(sizeof(char) * strlen(redir1_value) + 1);
        strcpy(redir1_info->target_filename, redir1_value);
        tree->redirections[0] = redir1_info;
    }
    
    next = peek_token(tokens);

    // Second redirection
    if (next && (get_token_type(next) == TOKEN_REDIR_IN || get_token_type(next) == TOKEN_REDIR_OUT)) {
        redir_type redir2_type = get_token_type(next) == TOKEN_REDIR_IN ? REDIR_IN : REDIR_OUT;
        consume_token(tokens);
        next = consume_token(tokens);
        if (get_token_type(next) != TOKEN_WORD) {
            free_parse_tree(tree);
            free_token_list_container(original_tokens);
            return error_tree("Redirection must be followed by a filename");
        }
        redir_info *redir2_info = malloc(sizeof(redir_info));
        redir2_info->type = redir2_type;
        char *redir2_value = get_token_value(next);
        redir2_info->target_filename = malloc(sizeof(char) * strlen(redir2_value) + 1);
        strcpy(redir2_info->target_filename, redir2_value);
        tree->redirections[1] = redir2_info;
    }

    free_token_list_container(original_tokens);
    return tree;
}

static void eat_newlines(lexer_token_list *tokens) {
    lexer_token *token = peek_token(tokens);
    while (token && get_token_type(token) == TOKEN_NEWLINE) {
        consume_token(tokens);
        token = peek_token(tokens);
    }
}

static parse_tree *parse_pipeline(lexer_token_list *tokens) {
    parse_tree *cmd1 = parse_command(tokens);
    lexer_token *next = peek_token(tokens);

    if (next && get_token_type(next) == TOKEN_PIPE) {
        consume_token(tokens);
        eat_newlines(tokens);
        parse_tree *pipe_rest = parse_pipeline(tokens);

        parse_tree *tree = init_tree();
        tree->type = PARSE_TREE_PIPE;
        tree->left = cmd1;
        tree->right = pipe_rest;
        return tree;
    }

    return cmd1;
}

static bool is_list_operator(token_type type) {
    return type == TOKEN_NEWLINE ||
           type == TOKEN_END_EXPR;
}

static bool is_command_list_operator(token_type type) {
    return type == TOKEN_AND ||
           type == TOKEN_OR ||
           type == TOKEN_BACKGROUND;
}

static parse_tree_type get_tree_type_for_token(token_type type) {
    switch(type) {
        case TOKEN_AND:
            return PARSE_TREE_AND;
        case TOKEN_OR:
            return PARSE_TREE_OR;
        case TOKEN_NEWLINE:
        case TOKEN_END_EXPR:
            return PARSE_TREE_LIST;
        case TOKEN_BACKGROUND:
            return PARSE_TREE_BACKGROUND;
        default:
            return PARSE_TREE_NONE;
    }
}

static bool requires_right_operand(parse_tree_type type) {
    return type == PARSE_TREE_AND ||
           type == PARSE_TREE_OR;
}

static parse_tree *parse_command_list(lexer_token_list *tokens) {
    parse_tree *pipe1 = parse_pipeline(tokens);
    lexer_token *next = peek_token(tokens);

    if (next && is_command_list_operator(get_token_type(next))) {
        consume_token(tokens);
        eat_newlines(tokens);
        parse_tree *command_list_rest = parse_command_list(tokens);

        parse_tree *tree = init_tree();
        tree->type = get_tree_type_for_token(get_token_type(next));
        tree->left = pipe1;
        tree->right = command_list_rest;
        return tree;
    }
    return pipe1;
}

static parse_tree *parse_list(lexer_token_list *tokens) {
    parse_tree *cmd_list1 = parse_command_list(tokens);
    lexer_token *next = peek_token(tokens);

    if (next && is_list_operator(get_token_type(next))) {
        consume_token(tokens);
        eat_newlines(tokens);
        parse_tree *list_rest = parse_list(tokens);

        parse_tree *tree = init_tree();
        tree->type = get_tree_type_for_token(get_token_type(next));
        tree->left = cmd_list1;
        tree->right = list_rest;

        return tree;
    }
    
    return cmd_list1;
}

parse_tree *parse(lexer_token_list *tokens) {
    // So that we don't destroy the original input
    lexer_token_list *copy = copy_token_list(tokens);
    parse_tree *tree = parse_list(copy);
    free_token_list_container(copy);
    return tree;
}

void free_parse_tree(parse_tree *tree) {
    if (tree->left) {
        free_parse_tree(tree->left);
    }
    if (tree->right) {
        free_parse_tree(tree->right);
    }
    while (tree->argc > 0) {
        tree->argc--;
        free(tree->argv[tree->argc]);
    }
    if (tree->redirections[0]) {
        free(tree->redirections[0]->target_filename);
        free(tree->redirections[0]);
    }
    if (tree->redirections[1]) {
        free(tree->redirections[1]->target_filename);
        free(tree->redirections[1]);
    }
    free(tree);
}
