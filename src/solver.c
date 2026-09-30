#include "solver.h"

#include <stdlib.h>
#include <string.h>

/* Neighbour order for both algorithms: up, right, down, left. */
#define NUM_DIRECTIONS 4
static const int ROW_OFFSET[NUM_DIRECTIONS] = { -1, 0, 1, 0 };
static const int COL_OFFSET[NUM_DIRECTIONS] = { 0, 1, 0, -1 };

/* Working memory for one search. Cells are addressed by index = row * cols + col. */
typedef struct {
    int cells;
    int *parent;              /* cell we came from; -1 for the start */
    unsigned char *visited;
    int *frontier;            /* BFS: queue of cells. DFS: stack of cells. */
    unsigned char *next_dir;  /* DFS only: next direction to try for each stack entry */
} SearchState;

void solve_result_init(SolveResult *result)
{
    result->algorithm = ALGO_DFS;
    result->ran = 0;
    result->found = 0;
    result->explored = 0;
    result->path_length = 0;
    result->path = NULL;
    result->path_count = 0;
}

void solve_result_free(SolveResult *result)
{
    free(result->path);
    solve_result_init(result);
}

const char *solver_algorithm_name(Algorithm algo)
{
    switch (algo) {
    case ALGO_DFS: return "DFS";
    case ALGO_BFS: return "BFS";
    case ALGO_COUNT: break;
    }
    return "?";
}

static void state_free(SearchState *st)
{
    free(st->parent);
    free(st->visited);
    free(st->frontier);
    free(st->next_dir);
    st->parent = NULL;
    st->visited = NULL;
    st->frontier = NULL;
    st->next_dir = NULL;
}

static int state_alloc(SearchState *st, int cells, Algorithm algo)
{
    size_t n = (size_t)cells;

    st->cells = cells;
    st->parent = malloc(n * sizeof *st->parent);
    st->visited = malloc(n * sizeof *st->visited);
    st->frontier = malloc(n * sizeof *st->frontier);
    st->next_dir = (algo == ALGO_DFS) ? malloc(n * sizeof *st->next_dir) : NULL;

    if (st->parent == NULL || st->visited == NULL || st->frontier == NULL ||
        (algo == ALGO_DFS && st->next_dir == NULL)) {
        state_free(st);
        return 0;
    }
    memset(st->visited, 0, n * sizeof *st->visited);
    return 1;
}

/* Index of the neighbouring cell in direction dir, or -1 if it is outside the
 * maze, a wall, or already visited. */
static int open_neighbour(const Maze *maze, const SearchState *st, int cell, int dir)
{
    int row = cell / maze->cols + ROW_OFFSET[dir];
    int col = cell % maze->cols + COL_OFFSET[dir];

    if (!maze_in_bounds(maze, row, col) || maze_cell_at(maze, row, col) == CELL_WALL) {
        return -1;
    }
    int idx = row * maze->cols + col;
    return st->visited[idx] ? -1 : idx;
}

/* Breadth-first: a cell is marked when first seen (enqueued) and counted as
 * explored when taken off the queue. Every cell enters the queue at most once,
 * so a queue of `cells` entries is enough. */
static int search_bfs(const Maze *maze, SearchState *st, int start, int goal, int *explored)
{
    int head = 0;
    int tail = 0;

    st->frontier[tail++] = start;
    st->visited[start] = 1;
    st->parent[start] = -1;

    while (head < tail) {
        int cur = st->frontier[head++];
        (*explored)++;
        if (cur == goal) {
            return 1;
        }
        for (int d = 0; d < NUM_DIRECTIONS; d++) {
            int next = open_neighbour(maze, st, cur, d);
            if (next >= 0) {
                st->visited[next] = 1;
                st->parent[next] = cur;
                st->frontier[tail++] = next;
            }
        }
    }
    return 0;
}

/* Depth-first with an explicit stack: follow one direction as far as possible,
 * and when a cell has no unvisited neighbours left, pop back to the previous
 * cell and try its next direction. A cell is marked and counted as explored
 * when it is pushed, so the stack never holds more than `cells` entries. */
static int search_dfs(const Maze *maze, SearchState *st, int start, int goal, int *explored)
{
    int depth = 0;

    st->frontier[depth] = start;
    st->next_dir[depth] = 0;
    depth++;
    st->visited[start] = 1;
    st->parent[start] = -1;
    (*explored)++;

    while (depth > 0) {
        int top = depth - 1;
        int cur = st->frontier[top];

        if (cur == goal) {
            return 1;
        }
        if (st->next_dir[top] >= NUM_DIRECTIONS) {
            depth--;            /* dead end: backtrack */
            continue;
        }
        int d = st->next_dir[top]++;
        int next = open_neighbour(maze, st, cur, d);
        if (next >= 0) {
            st->visited[next] = 1;
            st->parent[next] = cur;
            st->frontier[depth] = next;
            st->next_dir[depth] = 0;
            depth++;
            (*explored)++;
        }
    }
    return 0;
}

/* Walks parent links from the exit back to the start, then stores the cells
 * in start-to-exit order. Returns 0 if allocation fails. */
static int build_path(const Maze *maze, const SearchState *st, int goal, SolveResult *result)
{
    int count = 0;
    for (int cell = goal; cell != -1; cell = st->parent[cell]) {
        count++;
    }

    Position *path = malloc((size_t)count * sizeof *path);
    if (path == NULL) {
        return 0;
    }

    int i = count - 1;
    for (int cell = goal; cell != -1; cell = st->parent[cell]) {
        path[i].row = cell / maze->cols;
        path[i].col = cell % maze->cols;
        i--;
    }

    result->path = path;
    result->path_count = count;
    result->path_length = count - 1;
    return 1;
}

static int maze_is_solvable_input(const Maze *maze)
{
    if (maze->rows < 1 || maze->cols < 1 ||
        maze->rows > MAZE_MAX_ROWS || maze->cols > MAZE_MAX_COLS) {
        return 0;
    }
    if (!maze_in_bounds(maze, maze->start_pos.row, maze->start_pos.col) ||
        !maze_in_bounds(maze, maze->exit_pos.row, maze->exit_pos.col)) {
        return 0;
    }
    return maze_cell_at(maze, maze->start_pos.row, maze->start_pos.col) == CELL_START &&
           maze_cell_at(maze, maze->exit_pos.row, maze->exit_pos.col) == CELL_EXIT;
}

SolveStatus solver_run(const Maze *maze, Algorithm algo, SolveResult *result)
{
    SearchState st;
    int explored = 0;

    solve_result_init(result);
    if ((algo != ALGO_DFS && algo != ALGO_BFS) || !maze_is_solvable_input(maze)) {
        return SOLVE_INVALID_MAZE;
    }

    int start = maze->start_pos.row * maze->cols + maze->start_pos.col;
    int goal = maze->exit_pos.row * maze->cols + maze->exit_pos.col;

    if (!state_alloc(&st, maze->rows * maze->cols, algo)) {
        return SOLVE_OUT_OF_MEMORY;
    }

    int found = (algo == ALGO_BFS) ? search_bfs(maze, &st, start, goal, &explored)
                                   : search_dfs(maze, &st, start, goal, &explored);

    result->algorithm = algo;
    result->explored = explored;
    result->found = found;

    if (found && !build_path(maze, &st, goal, result)) {
        state_free(&st);
        solve_result_init(result);
        return SOLVE_OUT_OF_MEMORY;
    }

    state_free(&st);
    result->ran = 1;
    return SOLVE_OK;
}
