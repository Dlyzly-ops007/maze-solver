#ifndef MAZE_H
#define MAZE_H

/* Upper limits for the grid. Each maze file may use any size up to these. */
#define MAZE_MAX_ROWS 40
#define MAZE_MAX_COLS 60

/* Characters used in maze files and on screen. */
#define CHAR_WALL   '#'
#define CHAR_OPEN   '.'
#define CHAR_START  'S'
#define CHAR_EXIT   'E'
#define CHAR_PLAYER 'P'

typedef enum {
    CELL_WALL,
    CELL_OPEN,
    CELL_START,
    CELL_EXIT
} CellType;

typedef struct {
    int row;
    int col;
} Position;

typedef struct {
    CellType cells[MAZE_MAX_ROWS][MAZE_MAX_COLS];
    int rows;           /* actual size in use */
    int cols;
    Position start_pos; /* {-1, -1} until a start cell is found */
    Position exit_pos;
} Maze;

void maze_init(Maze *maze);
int maze_in_bounds(const Maze *maze, int row, int col);
CellType maze_cell_at(const Maze *maze, int row, int col);

/* Returns 1 and sets *out if c is a valid maze character, otherwise 0. */
int maze_cell_from_char(char c, CellType *out);
char maze_char_from_cell(CellType cell);

/* Prints the maze. If player_pos is not NULL, 'P' is drawn at that cell. */
void maze_display(const Maze *maze, const Position *player_pos);

#endif
