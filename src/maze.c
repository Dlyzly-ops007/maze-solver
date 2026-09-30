#include "maze.h"

#include <stdio.h>

void maze_init(Maze *maze)
{
    for (int r = 0; r < MAZE_MAX_ROWS; r++) {
        for (int c = 0; c < MAZE_MAX_COLS; c++) {
            maze->cells[r][c] = CELL_WALL;
        }
    }
    maze->rows = 0;
    maze->cols = 0;
    maze->start_pos.row = -1;
    maze->start_pos.col = -1;
    maze->exit_pos.row = -1;
    maze->exit_pos.col = -1;
}

int maze_in_bounds(const Maze *maze, int row, int col)
{
    return row >= 0 && row < maze->rows && col >= 0 && col < maze->cols;
}

CellType maze_cell_at(const Maze *maze, int row, int col)
{
    return maze->cells[row][col];
}

int maze_cell_from_char(char c, CellType *out)
{
    switch (c) {
    case CHAR_WALL:  *out = CELL_WALL;  return 1;
    case CHAR_OPEN:
    case CHAR_PATH:  *out = CELL_OPEN;  return 1;
    case CHAR_START: *out = CELL_START; return 1;
    case CHAR_EXIT:  *out = CELL_EXIT;  return 1;
    default:         return 0;
    }
}

char maze_char_from_cell(CellType cell)
{
    switch (cell) {
    case CELL_WALL:  return CHAR_WALL;
    case CELL_OPEN:  return CHAR_OPEN;
    case CELL_START: return CHAR_START;
    case CELL_EXIT:  return CHAR_EXIT;
    }
    return '?';
}

void maze_mark_path(const Maze *maze, const Position *cells, int count, PathMarks marks)
{
    for (int r = 0; r < MAZE_MAX_ROWS; r++) {
        for (int c = 0; c < MAZE_MAX_COLS; c++) {
            marks[r][c] = 0;
        }
    }
    for (int i = 0; i < count; i++) {
        if (maze_in_bounds(maze, cells[i].row, cells[i].col)) {
            marks[cells[i].row][cells[i].col] = 1;
        }
    }
}

char maze_symbol_at(const Maze *maze, int row, int col, PathMarks marks)
{
    CellType cell = maze->cells[row][col];
    if (marks != NULL && marks[row][col] && cell == CELL_OPEN) {
        return CHAR_PATH;
    }
    return maze_char_from_cell(cell);
}

static void print_grid(const Maze *maze, const Position *player_pos, PathMarks marks)
{
    putchar('\n');
    for (int r = 0; r < maze->rows; r++) {
        for (int c = 0; c < maze->cols; c++) {
            char ch = maze_symbol_at(maze, r, c, marks);
            if (player_pos != NULL && player_pos->row == r && player_pos->col == c) {
                ch = CHAR_PLAYER;
            }
            putchar(ch);
            /* A space between cells makes the grid look closer to square. */
            putchar(c == maze->cols - 1 ? '\n' : ' ');
        }
    }
}

void maze_display(const Maze *maze, const Position *player_pos)
{
    print_grid(maze, player_pos, NULL);
    printf("\n%c wall  %c open  %c start  %c exit  %c you\n",
           CHAR_WALL, CHAR_OPEN, CHAR_START, CHAR_EXIT, CHAR_PLAYER);
}

void maze_display_path(const Maze *maze, const Position *cells, int count)
{
    PathMarks marks;
    maze_mark_path(maze, cells, count, marks);
    print_grid(maze, NULL, marks);
    printf("\n%c wall  %c open  %c start  %c exit  %c solution\n",
           CHAR_WALL, CHAR_OPEN, CHAR_START, CHAR_EXIT, CHAR_PATH);
}
