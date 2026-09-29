#include "maze_loader.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "validation.h"

#define LINE_BUF_SIZE 256

static int parse_maze_stream(FILE *fp, Maze *maze, char *err, size_t err_size)
{
    char line[LINE_BUF_SIZE];
    int line_no = 0;
    int saw_blank = 0;

    maze_init(maze);

    while (fgets(line, sizeof line, fp) != NULL) {
        line_no++;
        size_t len = strlen(line);

        if (len > 0 && line[len - 1] == '\n') {
            line[--len] = '\0';
        } else if (!feof(fp)) {
            snprintf(err, err_size, "line %d is too long", line_no);
            return 0;
        }
        if (len > 0 && line[len - 1] == '\r') {
            line[--len] = '\0';
        }

        /* Blank lines are fine at the end of the file, but not between rows. */
        if (len == 0) {
            saw_blank = 1;
            continue;
        }
        if (saw_blank) {
            snprintf(err, err_size, "unexpected blank line before line %d", line_no);
            return 0;
        }

        if (maze->rows >= MAZE_MAX_ROWS) {
            snprintf(err, err_size, "too many rows (maximum is %d)", MAZE_MAX_ROWS);
            return 0;
        }
        if (len > MAZE_MAX_COLS) {
            snprintf(err, err_size, "line %d is wider than the maximum of %d columns",
                     line_no, MAZE_MAX_COLS);
            return 0;
        }
        if (maze->rows == 0) {
            maze->cols = (int)len;
        } else if ((int)len != maze->cols) {
            snprintf(err, err_size, "line %d has %d columns, expected %d",
                     line_no, (int)len, maze->cols);
            return 0;
        }

        for (int c = 0; c < maze->cols; c++) {
            CellType cell;
            if (!maze_cell_from_char(line[c], &cell)) {
                if (isprint((unsigned char)line[c])) {
                    snprintf(err, err_size, "line %d, column %d: invalid character '%c'",
                             line_no, c + 1, line[c]);
                } else {
                    snprintf(err, err_size, "line %d, column %d: invalid non-printable character",
                             line_no, c + 1);
                }
                return 0;
            }
            maze->cells[maze->rows][c] = cell;

            /* Remember the first start/exit; validation checks for duplicates. */
            if (cell == CELL_START && maze->start_pos.row < 0) {
                maze->start_pos.row = maze->rows;
                maze->start_pos.col = c;
            } else if (cell == CELL_EXIT && maze->exit_pos.row < 0) {
                maze->exit_pos.row = maze->rows;
                maze->exit_pos.col = c;
            }
        }
        maze->rows++;
    }

    if (ferror(fp)) {
        snprintf(err, err_size, "error while reading file");
        return 0;
    }
    return validation_check_maze(maze, err, err_size);
}

int maze_load_from_file(const char *path, Maze *maze, char *err, size_t err_size)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        snprintf(err, err_size, "cannot open '%s': %s", path, strerror(errno));
        return 0;
    }

    int ok = parse_maze_stream(fp, maze, err, err_size);
    fclose(fp);
    return ok;
}
