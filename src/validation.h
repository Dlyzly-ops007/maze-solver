#ifndef VALIDATION_H
#define VALIDATION_H

#include <stddef.h>

#include "maze.h"
#include "player.h"

#define INPUT_BUF_SIZE 256

/* Checks a freshly loaded maze: size limits, exactly one start and one exit,
 * and that the recorded start/exit positions sit on matching cells.
 * Returns 1 if valid. Otherwise returns 0 and writes a message to err. */
int validation_check_maze(const Maze *maze, char *err, size_t err_size);

/* Reads one line from stdin into buf, trimmed of surrounding whitespace.
 * Overlong input is truncated and the rest of the line is discarded.
 * Returns 0 on end of input. */
int input_read_line(char *buf, size_t size);

/* Return the menu number (1..max_choice), or 0 if the line is not valid. */
int input_parse_menu_choice(const char *line, int max_choice);

/* W/A/S/D in either case. Returns 1 and sets *dir, or 0 if not a move. */
int input_parse_direction(const char *line, Direction *dir);

/* Returns 1 if the line is Q/q. */
int input_is_quit(const char *line);

#endif
