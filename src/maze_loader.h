#ifndef MAZE_LOADER_H
#define MAZE_LOADER_H

#include <stddef.h>

#include "maze.h"

#define MAZE_ERROR_SIZE 384

/* Loads and validates a maze file. On success returns 1 and fills *maze.
 * On failure returns 0, writes a readable message to err, and leaves *maze
 * in an unspecified state (callers should discard it). */
int maze_load_from_file(const char *path, Maze *maze, char *err, size_t err_size);

#endif
