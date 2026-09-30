#ifndef SOLVER_H
#define SOLVER_H

#include "maze.h"

typedef enum {
    ALGO_DFS,
    ALGO_BFS,
    ALGO_COUNT
} Algorithm;

typedef enum {
    SOLVE_OK,            /* search finished (path found or proven absent) */
    SOLVE_INVALID_MAZE,
    SOLVE_OUT_OF_MEMORY
} SolveStatus;

typedef struct {
    Algorithm algorithm;
    int ran;           /* 1 once a search has completed */
    int found;         /* 1 if the exit is reachable */
    int explored;      /* cells visited by the search, including start (and exit if found) */
    int path_length;   /* moves from start to exit (path_count - 1); 0 if not found */
    Position *path;    /* malloc'd, start..exit inclusive; NULL if not found */
    int path_count;
} SolveResult;

void solve_result_init(SolveResult *result);

/* Frees the path and resets the result. Safe to call repeatedly. */
void solve_result_free(SolveResult *result);

const char *solver_algorithm_name(Algorithm algo);

/* Searches from the maze's start to its exit without modifying the maze.
 * Moves are up/down/left/right through non-wall cells.
 * On SOLVE_OK, *result is filled in (caller must call solve_result_free).
 * On any other status, *result is left empty and owns no memory.
 * *result must not already own a path: it is overwritten, not freed. */
SolveStatus solver_run(const Maze *maze, Algorithm algo, SolveResult *result);

#endif
