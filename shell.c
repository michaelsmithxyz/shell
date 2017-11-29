#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "exec.h"
#include "input.h"
#include "parser.h"
#include "tokens.h"
#include "util.h"

void do_command(char *command) {
    // For now, display tokens
    lexer_token_list *token_list = init_token_list();
    lexer_context *lexer = init_lexer(command);
    lexer_token *token = next_token(lexer);
    while (token) {
        add_token(token_list, token);
        token = next_token(lexer);
    }
    free_lexer(lexer);
    parse_tree *tree = parse(token_list);
    if (tree) {
        if (tree->type == PARSE_TREE_ERROR) {
            fprintf(stderr, "%s\n", tree->argv[0]);
        } else if (tree->type != PARSE_TREE_NONE) {
            exec_tree(tree);
        }
        free_parse_tree(tree);
    }
    free_token_list(token_list);
}

// Main interactive mode loop
static void repl(void) {
    char input[1024];
    bool run = true;
    while (run) {
        string_buffer *input_buffer = init_string_buffer();
        input_line *line = NULL;
        bool more = false;
        
        do {
            char *prompt = line == NULL ? "$ " : "> ";
            line = get_line(stdin, prompt);
            if (line == NULL) {
                run = false;
                break;
            }
            push_string(input_buffer, line_content(line));
            more = line_hasmore(line);
            free_input_line(line);
        } while(more);
        
        char *raw_input = build_string(input_buffer);

        do_command(raw_input);

        free_string_buffer(input_buffer);
        free(raw_input);
    }
}

static void script(char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Unable to open script file\n");
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *contents = malloc(sizeof(char) * size + 1);
    size_t read_size = fread(contents, size, 1, file);
    fclose(file);
    contents[size] = '\0';

    if (!read_size) {
        fprintf(stderr, "Error reading script file\n");
        free(contents);
        exit(1);
    }

    do_command(contents);
    free(contents);
}

static void usage(void) {
    fprintf(stderr, "Usage: nush [FILE]\n");
}

int main(int argc, char **argv) {
    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        script(argv[1]);
    } else {
        usage();
    }
}
