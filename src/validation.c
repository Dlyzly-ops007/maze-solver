#include "validation.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

int validation_check_maze(const Maze *maze, char *err, size_t err_size)
{
    if (maze->rows < 1 || maze->cols < 1) {
        snprintf(err, err_size, "maze is empty");
        return 0;
    }
    if (maze->rows > MAZE_MAX_ROWS || maze->cols > MAZE_MAX_COLS) {
        snprintf(err, err_size, "maze is larger than %d rows x %d columns",
                 MAZE_MAX_ROWS, MAZE_MAX_COLS);
        return 0;
    }

    int starts = 0;
    int exits = 0;
    for (int r = 0; r < maze->rows; r++) {
        for (int c = 0; c < maze->cols; c++) {
            CellType cell = maze_cell_at(maze, r, c);
            if (cell == CELL_START) {
                starts++;
            } else if (cell == CELL_EXIT) {
                exits++;
            }
        }
    }

    if (starts != 1) {
        snprintf(err, err_size, "expected exactly one start '%c', found %d",
                 CHAR_START, starts);
        return 0;
    }
    if (exits != 1) {
        snprintf(err, err_size, "expected exactly one exit '%c', found %d",
                 CHAR_EXIT, exits);
        return 0;
    }

    if (!maze_in_bounds(maze, maze->start_pos.row, maze->start_pos.col) ||
        maze_cell_at(maze, maze->start_pos.row, maze->start_pos.col) != CELL_START) {
        snprintf(err, err_size, "start position is not on a valid cell");
        return 0;
    }
    if (!maze_in_bounds(maze, maze->exit_pos.row, maze->exit_pos.col) ||
        maze_cell_at(maze, maze->exit_pos.row, maze->exit_pos.col) != CELL_EXIT) {
        snprintf(err, err_size, "exit position is not on a valid cell");
        return 0;
    }
    return 1;
}

static void trim_in_place(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[--len] = '\0';
    }
    size_t lead = 0;
    while (s[lead] != '\0' && isspace((unsigned char)s[lead])) {
        lead++;
    }
    if (lead > 0) {
        memmove(s, s + lead, strlen(s + lead) + 1);
    }
}

int input_read_line(char *buf, size_t size)
{
    if (fgets(buf, (int)size, stdin) == NULL) {
        return 0;
    }

    size_t len = strlen(buf);
    if (len == 0 || buf[len - 1] != '\n') {
        /* No newline: either the line was longer than buf or we hit EOF.
         * Discard whatever is left so it isn't read as the next command. */
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {
        }
    }
    trim_in_place(buf);
    return 1;
}

int input_parse_menu_choice(const char *line, int max_choice)
{
    if (strlen(line) != 1 || !isdigit((unsigned char)line[0])) {
        return 0;
    }
    int choice = line[0] - '0';
    return (choice >= 1 && choice <= max_choice) ? choice : 0;
}

int input_parse_direction(const char *line, Direction *dir)
{
    if (strlen(line) != 1) {
        return 0;
    }
    switch (tolower((unsigned char)line[0])) {
    case 'w': *dir = DIR_UP;    return 1;
    case 'a': *dir = DIR_LEFT;  return 1;
    case 's': *dir = DIR_DOWN;  return 1;
    case 'd': *dir = DIR_RIGHT; return 1;
    default:  return 0;
    }
}

int input_is_quit(const char *line)
{
    return strlen(line) == 1 && tolower((unsigned char)line[0]) == 'q';
}

int input_is_yes(const char *line)
{
    if (strlen(line) == 1) {
        return tolower((unsigned char)line[0]) == 'y';
    }
    return strlen(line) == 3 &&
           tolower((unsigned char)line[0]) == 'y' &&
           tolower((unsigned char)line[1]) == 'e' &&
           tolower((unsigned char)line[2]) == 's';
}
