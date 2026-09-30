#ifndef MAZE_LOADER_H
#define MAZE_LOADER_H

#include <stddef.h>

#include "maze.h"

#define MAZE_ERROR_SIZE 384

typedef enum {
    SAVE_OK,
    SAVE_EXISTS,   /* file already exists and overwrite was not requested */
    SAVE_ERROR
} SaveStatus;

/* Loads and validates a maze file. On success returns 1 and fills *maze.
 * On failure returns 0, writes a readable message to err, and leaves *maze
 * in an unspecified state (callers should discard it). */
int maze_load_from_file(const char *path, Maze *maze, char *err, size_t err_size);

/* Writes the maze to a file, drawing the given solution cells as '*'.
 * Pass count = 0 to save the plain maze. Without overwrite, an existing file
 * is left untouched and SAVE_EXISTS is returned. The saved file can be loaded
 * again ('*' loads as an open cell). */
SaveStatus maze_save_to_file(const char *path, const Maze *maze,
                             const Position *solution, int count, int overwrite,
                             char *err, size_t err_size);

#endif
