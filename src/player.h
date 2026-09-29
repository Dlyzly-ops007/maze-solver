#ifndef PLAYER_H
#define PLAYER_H

#include "maze.h"

typedef enum {
    DIR_UP,
    DIR_LEFT,
    DIR_DOWN,
    DIR_RIGHT
} Direction;

typedef enum {
    MOVE_OK,
    MOVE_BLOCKED_WALL,
    MOVE_OUT_OF_BOUNDS,
    MOVE_REACHED_EXIT,
    MOVE_ALREADY_FINISHED
} MoveResult;

typedef struct {
    Position pos;
    int moves;      /* successful moves only */
    int finished;   /* set once the exit is reached */
} Player;

/* Puts the player on the maze's start cell and clears move count/state. */
void player_reset(Player *player, const Maze *maze);

/* Tries to move one cell. The player only changes if the result is
 * MOVE_OK or MOVE_REACHED_EXIT. */
MoveResult player_move(Player *player, const Maze *maze, Direction dir);

#endif
