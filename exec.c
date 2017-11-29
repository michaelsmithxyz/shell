#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include <sys/wait.h>

#include "exec.h"


#define BUILTIN_CD "cd"
#define BUILTIN_EXIT "exit"


struct execution_context {
    int outfd;
    int infd;
    bool wait;
};
typedef struct execution_context execution_context;


static bool exec_tree_real(parse_tree *tree, execution_context context);


bool is_builtin(char **argv) {
    return strcmp(argv[0], BUILTIN_CD) == 0 ||
           strcmp(argv[0], BUILTIN_EXIT) == 0;
}

static bool builtin_cd(char **argv) {
    if (!argv[1]) {
        return false;
    }
    return chdir(argv[1]) == 0;
}

static bool builtin_exit(char **argv) {
    if (argv[1]) {
        exit(atoi(argv[1]));
    }
    exit(0);
    return true;
}

bool exec_builtin(parse_tree *tree) {
    if (tree->type != PARSE_TREE_COMMAND || !is_builtin(tree->argv)) {
        return false;
    }

    char **argv = tree->argv;
    char *cmd = tree->argv[0];

    if (!strcmp(cmd, BUILTIN_CD)) {
        return builtin_cd(argv);
    } else if (!strcmp(cmd, BUILTIN_EXIT)) {
        return builtin_exit(argv);
    }
    return false;
}


static int do_exec(char **argv, execution_context context) {
    pid_t child;
    if((child = fork()) == 0) {
        // Child process
        if (context.outfd != STDOUT_FILENO) {
            dup2(context.outfd, STDOUT_FILENO);
            close(context.outfd);
        }
        if (context.infd != STDIN_FILENO) {
            dup2(context.infd, STDIN_FILENO);
            close(context.infd);
        }

        execvp(argv[0], argv);
        exit(1);
    } else {
        // Parent
        if (context.wait) {
            int status;
            waitpid(child, &status, 0);
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            }
        }
    }
    return 0;
}

static bool apply_redirection(execution_context *context, redir_info *redirection) {
    switch (redirection->type) {
        case REDIR_OUT: ;
            int outfd = open(redirection->target_filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
            if (outfd == -1) {
                perror("Error: ");
                // TODO: Handle errors
                return false;
            }
            context->outfd = outfd;
            break;
        case REDIR_IN: ;
            int infd = open(redirection->target_filename, O_RDONLY);
            if (infd == -1) {
                perror("Error: ");
                // TODO: Handle errors
                return false;
            }
            context->infd = infd;
            break;
    }
    return true;
}

static bool subshell_exec_tree_real(parse_tree *tree, execution_context context) {
    pid_t child;
    execution_context child_context = context;
    bool wait = context.wait;
    child_context.wait = true;
    if ((child = fork()) == 0) {
        // Child
        exit(exec_tree_real(tree, child_context) ? 0 : 1);
    } else {
        if (context.wait) {
            int status;
            waitpid(child, &status, 0);
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status) == 0;
            }
        }
    }
    return true;
}


static bool exec_tree_real(parse_tree *tree, execution_context context) {
    if (tree->type == PARSE_TREE_LIST) {
        exec_tree_real(tree->left, context);
        if (tree->right) {
            return exec_tree_real(tree->right, context);
        }
    }

    if (tree->type == PARSE_TREE_OR) {
        execution_context left_context = context;
        left_context.wait = true;
        bool left_status = exec_tree_real(tree->left, left_context);
        if (!left_status) {
            return  exec_tree_real(tree->right, context);
        }
        return true;
    }

    if (tree->type == PARSE_TREE_AND) {
        execution_context left_context = context;
        left_context.wait = true;
        bool left_status = exec_tree_real(tree->left, left_context);
        if (left_status) {
            return exec_tree_real(tree->right, context);
        }
        return false;
    }

    if (tree->type == PARSE_TREE_BACKGROUND) {
        execution_context background_context = context;
        background_context.wait = false;
        subshell_exec_tree_real(tree->left, background_context);
        if (tree->right) {
            return exec_tree_real(tree->right, context);
        }
        return true;
    }

    if (tree->type == PARSE_TREE_COMMAND) {
        char **argv = tree->argv;
        
        if (is_builtin(argv)) {
            return exec_builtin(tree);
        }

        execution_context child_context = context;
        if (tree->redirections[0]) {
            apply_redirection(&child_context, tree->redirections[0]);
            if (tree->redirections[1]) {
                apply_redirection(&child_context, tree->redirections[0]);
            }
        }

        return do_exec(argv, child_context) == 0;
    }

    if (tree->type == PARSE_TREE_PIPE) {
        int pipes[2];
        pipe(pipes);

        int out_pipe = pipes[1];
        execution_context left_context = context;
        left_context.outfd = out_pipe;
        left_context.wait = false;

        int in_pipe = pipes[0];
        execution_context right_context = context;
        right_context.infd = in_pipe;

        exec_tree_real(tree->left, left_context);
        close(out_pipe);
        return exec_tree_real(tree->right, right_context);
    }

    if (tree->type == PARSE_TREE_ERROR) {
        fprintf(stderr, "Fatal Error: Executing error parse tree\n");
    }

    return false;
}

bool exec_tree(parse_tree *tree) {
    execution_context context;
    context.outfd = STDOUT_FILENO;
    context.infd = STDIN_FILENO;
    context.wait = true;
    return exec_tree_real(tree, context);
}
