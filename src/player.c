#include "player.h"

/* Indexed by Direction. */
static const int ROW_STEP[] = { -1, 0, 1, 0 };
static const int COL_STEP[] = { 0, -1, 0, 1 };

void player_reset(Player *player, const Maze *maze)
{
    player->pos = maze->start_pos;
    player->moves = 0;
    player->finished = 0;
}

MoveResult player_move(Player *player, const Maze *maze, Direction dir)
{
    if (player->finished) {
        return MOVE_ALREADY_FINISHED;
    }

    int new_row = player->pos.row + ROW_STEP[dir];
    int new_col = player->pos.col + COL_STEP[dir];

    if (!maze_in_bounds(maze, new_row, new_col)) {
        return MOVE_OUT_OF_BOUNDS;
    }

    CellType target = maze_cell_at(maze, new_row, new_col);
    if (target == CELL_WALL) {
        return MOVE_BLOCKED_WALL;
    }

    player->pos.row = new_row;
    player->pos.col = new_col;
    player->moves++;

    if (target == CELL_EXIT) {
        player->finished = 1;
        return MOVE_REACHED_EXIT;
    }
    return MOVE_OK;
}
