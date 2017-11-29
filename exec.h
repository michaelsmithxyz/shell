#pragma once

#include "parser.h"

bool is_builtin(char **argv);

bool exec_builtin(parse_tree *tree);

bool exec_tree(parse_tree *tree);
