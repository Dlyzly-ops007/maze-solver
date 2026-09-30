/* Unit tests for the solver. Built and run by `make test`.
 *
 * Checks path validity directly (start, exit, adjacency, no walls) and
 * cross-checks BFS against an independent shortest-distance calculation on
 * thousands of random mazes. Allocation failures are simulated by wrapping
 * malloc at link time (-Wl,--wrap=malloc). */
#include <stdio.h>
#include <stdlib.h>

#include "maze.h"
#include "maze_loader.h"
#include "solver.h"

/* ---------- malloc failure injection ---------- */

static int allocs_before_failure = -1;   /* -1: never fail */

void *__real_malloc(size_t size);
void *__wrap_malloc(size_t size)
{
    if (allocs_before_failure == 0) {
        return NULL;
    }
    if (allocs_before_failure > 0) {
        allocs_before_failure--;
    }
    return __real_malloc(size);
}

/* ---------- tiny test framework ---------- */

static int failures = 0;
static int checks = 0;

#define CHECK(cond, ...)                                              \
    do {                                                              \
        checks++;                                                     \
        if (!(cond)) {                                                \
            failures++;                                               \
            printf("  FAIL %s:%d: ", __FILE__, __LINE__);             \
            printf(__VA_ARGS__);                                      \
            printf("\n");                                             \
        }                                                             \
    } while (0)

static void section(const char *name)
{
    printf("%s\n", name);
}

/* ---------- helpers ---------- */

static int load(Maze *maze, const char *path)
{
    char err[MAZE_ERROR_SIZE];
    if (!maze_load_from_file(path, maze, err, sizeof err)) {
        printf("  cannot load %s: %s\n", path, err);
        return 0;
    }
    return 1;
}

/* A path is valid if it starts at S, ends at E, moves one orthogonal step at a
 * time, stays in bounds, never touches a wall, and never repeats a cell. */
static int path_is_valid(const Maze *maze, const SolveResult *res)
{
    PathMarks seen;
    if (res->path == NULL || res->path_count < 2 || res->path_length != res->path_count - 1) {
        return 0;
    }
    maze_mark_path(maze, NULL, 0, seen);
    for (int i = 0; i < res->path_count; i++) {
        Position p = res->path[i];
        if (!maze_in_bounds(maze, p.row, p.col) || maze_cell_at(maze, p.row, p.col) == CELL_WALL) {
            return 0;
        }
        if (seen[p.row][p.col]) {
            return 0;
        }
        seen[p.row][p.col] = 1;
        if (i > 0) {
            int dr = abs(p.row - res->path[i - 1].row);
            int dc = abs(p.col - res->path[i - 1].col);
            if (dr + dc != 1) {
                return 0;
            }
        }
    }
    return res->path[0].row == maze->start_pos.row && res->path[0].col == maze->start_pos.col &&
           res->path[res->path_count - 1].row == maze->exit_pos.row &&
           res->path[res->path_count - 1].col == maze->exit_pos.col;
}

/* Independent shortest-distance reference: repeatedly relax distances until
 * nothing changes (no queue, no shared code with the solver).
 * Returns -1 if the exit is unreachable; *reachable gets the number of cells
 * reachable from the start. */
static int reference_distance(const Maze *maze, int *reachable)
{
    static int dist[MAZE_MAX_ROWS][MAZE_MAX_COLS];
    const int INF = 1 << 28;
    int changed = 1;

    for (int r = 0; r < maze->rows; r++)
        for (int c = 0; c < maze->cols; c++)
            dist[r][c] = INF;
    dist[maze->start_pos.row][maze->start_pos.col] = 0;

    while (changed) {
        changed = 0;
        for (int r = 0; r < maze->rows; r++) {
            for (int c = 0; c < maze->cols; c++) {
                if (maze_cell_at(maze, r, c) == CELL_WALL) continue;
                static const int dr[4] = { -1, 1, 0, 0 };
                static const int dc[4] = { 0, 0, -1, 1 };
                for (int k = 0; k < 4; k++) {
                    int nr = r + dr[k], nc = c + dc[k];
                    if (maze_in_bounds(maze, nr, nc) && dist[nr][nc] + 1 < dist[r][c]) {
                        dist[r][c] = dist[nr][nc] + 1;
                        changed = 1;
                    }
                }
            }
        }
    }

    int count = 0;
    for (int r = 0; r < maze->rows; r++)
        for (int c = 0; c < maze->cols; c++)
            if (dist[r][c] < INF) count++;
    if (reachable != NULL) *reachable = count;

    int d = dist[maze->exit_pos.row][maze->exit_pos.col];
    return d >= INF ? -1 : d;
}

static void solve_both(const Maze *maze, SolveResult *dfs, SolveResult *bfs)
{
    CHECK(solver_run(maze, ALGO_DFS, dfs) == SOLVE_OK, "DFS run failed");
    CHECK(solver_run(maze, ALGO_BFS, bfs) == SOLVE_OK, "BFS run failed");
}

/* ---------- tests ---------- */

static void test_simple(void)
{
    section("simple solvable maze (mazes/small.txt)");
    Maze m;
    SolveResult dfs, bfs;
    if (!load(&m, "mazes/small.txt")) { failures++; return; }
    solve_both(&m, &dfs, &bfs);

    CHECK(dfs.found && bfs.found, "both should find the exit");
    CHECK(path_is_valid(&m, &dfs), "DFS path invalid");
    CHECK(path_is_valid(&m, &bfs), "BFS path invalid");
    CHECK(dfs.path_length == 7 && bfs.path_length == 7, "only one route, expected 7 moves");
    CHECK(dfs.algorithm == ALGO_DFS && bfs.algorithm == ALGO_BFS, "algorithm not recorded");

    /* the maze itself must be unchanged by solving */
    CHECK(maze_cell_at(&m, 1, 1) == CELL_START && maze_cell_at(&m, 3, 6) == CELL_EXIT &&
          maze_cell_at(&m, 1, 2) == CELL_OPEN, "maze was modified");
    solve_result_free(&dfs);
    solve_result_free(&bfs);
}

static void test_multiple_paths(void)
{
    section("multiple paths: BFS shortest, DFS separate (mazes/two_routes.txt)");
    Maze m;
    SolveResult dfs, bfs;
    if (!load(&m, "mazes/two_routes.txt")) { failures++; return; }
    solve_both(&m, &dfs, &bfs);

    CHECK(path_is_valid(&m, &dfs) && path_is_valid(&m, &bfs), "invalid path");
    CHECK(bfs.path_length == 10, "BFS length %d, expected 10", bfs.path_length);
    CHECK(bfs.path_length == reference_distance(&m, NULL), "BFS is not the shortest distance");
    CHECK(dfs.path_length == 14, "DFS length %d, expected 14 (documented neighbour order)", dfs.path_length);
    CHECK(dfs.path_length >= bfs.path_length, "DFS shorter than the shortest path?");
    solve_result_free(&dfs);
    solve_result_free(&bfs);
}

static void test_backtracking(void)
{
    section("backtracking (tests/mazes/backtrack.txt)");
    Maze m;
    SolveResult dfs, bfs;
    if (!load(&m, "tests/mazes/backtrack.txt")) { failures++; return; }
    solve_both(&m, &dfs, &bfs);

    CHECK(dfs.found && path_is_valid(&m, &dfs), "DFS should still find a valid path");
    /* DFS tries 'up' first from S, enters the dead-end pocket, then backs out. */
    CHECK(dfs.explored > dfs.path_count, "DFS explored %d, path cells %d: expected a dead end visit",
          dfs.explored, dfs.path_count);
    CHECK(bfs.path_length == 4, "BFS length %d, expected 4", bfs.path_length);
    solve_result_free(&dfs);
    solve_result_free(&bfs);
}

static void test_no_solution(void)
{
    section("no solution (tests/mazes/unsolvable.txt)");
    Maze m;
    SolveResult dfs, bfs;
    int reachable = 0;
    if (!load(&m, "tests/mazes/unsolvable.txt")) { failures++; return; }
    (void)reference_distance(&m, &reachable);
    solve_both(&m, &dfs, &bfs);

    CHECK(!dfs.found && !bfs.found, "should not find a path");
    CHECK(dfs.path == NULL && bfs.path == NULL, "no path should be allocated");
    CHECK(dfs.ran && bfs.ran, "ran flag should be set");
    CHECK(dfs.explored == reachable && bfs.explored == reachable,
          "both should explore every reachable cell (%d): DFS %d, BFS %d",
          reachable, dfs.explored, bfs.explored);
    solve_result_free(&dfs);
    solve_result_free(&bfs);
}

static void test_adjacent(void)
{
    section("start next to exit (tests/mazes/adjacent.txt)");
    Maze m;
    SolveResult dfs, bfs;
    if (!load(&m, "tests/mazes/adjacent.txt")) { failures++; return; }
    solve_both(&m, &dfs, &bfs);

    CHECK(dfs.path_length == 1 && bfs.path_length == 1, "expected one move");
    CHECK(dfs.path_count == 2 && bfs.path_count == 2, "expected two path cells");
    CHECK(path_is_valid(&m, &dfs) && path_is_valid(&m, &bfs), "invalid path");
    solve_result_free(&dfs);
    solve_result_free(&bfs);
}

static void test_large(void)
{
    section("larger maze (mazes/large.txt, 39 x 59)");
    Maze m;
    SolveResult dfs, bfs;
    if (!load(&m, "mazes/large.txt")) { failures++; return; }
    CHECK(m.rows == 39 && m.cols == 59, "unexpected size %d x %d", m.rows, m.cols);
    solve_both(&m, &dfs, &bfs);

    CHECK(dfs.found && bfs.found, "should be solvable");
    CHECK(path_is_valid(&m, &dfs) && path_is_valid(&m, &bfs), "invalid path");
    CHECK(bfs.path_length == reference_distance(&m, NULL), "BFS is not the shortest distance");
    CHECK(dfs.explored <= m.rows * m.cols && bfs.explored <= m.rows * m.cols, "explored more cells than exist");
    solve_result_free(&dfs);
    solve_result_free(&bfs);
}

static void test_repeated(void)
{
    section("repeated solving");
    Maze m;
    if (!load(&m, "mazes/sample.txt")) { failures++; return; }

    SolveResult first, again;
    for (int algo = 0; algo < ALGO_COUNT; algo++) {
        CHECK(solver_run(&m, (Algorithm)algo, &first) == SOLVE_OK, "first run failed");
        for (int i = 0; i < 50; i++) {
            CHECK(solver_run(&m, (Algorithm)algo, &again) == SOLVE_OK, "repeat run failed");
            CHECK(again.path_length == first.path_length && again.explored == first.explored,
                  "repeat run gave different numbers");
            solve_result_free(&again);
        }
        solve_result_free(&first);
    }
    solve_result_free(&first);   /* double free must be harmless */
}

/* Small deterministic generator so results do not depend on the platform's rand(). */
static unsigned long rng_state = 12345;
static int rng(int bound)
{
    rng_state = rng_state * 1103515245UL + 12345UL;
    return (int)((rng_state >> 16) % (unsigned long)bound);
}

static void test_random(void)
{
    section("3000 random mazes: BFS vs reference, DFS validity, agreement");
    int solvable = 0, unsolvable = 0, bad = 0;

    for (int iter = 0; iter < 3000; iter++) {
        Maze m;
        maze_init(&m);
        m.rows = 3 + rng(8);
        m.cols = 3 + rng(10);
        for (int r = 0; r < m.rows; r++)
            for (int c = 0; c < m.cols; c++)
                m.cells[r][c] = rng(100) < 35 ? CELL_WALL : CELL_OPEN;

        m.start_pos.row = rng(m.rows); m.start_pos.col = rng(m.cols);
        do {
            m.exit_pos.row = rng(m.rows); m.exit_pos.col = rng(m.cols);
        } while (m.exit_pos.row == m.start_pos.row && m.exit_pos.col == m.start_pos.col);
        m.cells[m.start_pos.row][m.start_pos.col] = CELL_START;
        m.cells[m.exit_pos.row][m.exit_pos.col] = CELL_EXIT;

        int reachable = 0;
        int expected = reference_distance(&m, &reachable);
        SolveResult dfs, bfs;
        if (solver_run(&m, ALGO_DFS, &dfs) != SOLVE_OK || solver_run(&m, ALGO_BFS, &bfs) != SOLVE_OK) {
            bad++;
            continue;
        }

        int ok = (dfs.found == bfs.found) && (bfs.found == (expected >= 0));
        if (ok && expected >= 0) {
            ok = path_is_valid(&m, &bfs) && path_is_valid(&m, &dfs) &&
                 bfs.path_length == expected && dfs.path_length >= expected;
            solvable++;
        } else if (ok) {
            ok = dfs.path == NULL && bfs.path == NULL &&
                 dfs.explored == reachable && bfs.explored == reachable;
            unsolvable++;
        }
        if (!ok) bad++;
        solve_result_free(&dfs);
        solve_result_free(&bfs);
    }
    CHECK(bad == 0, "%d random mazes disagreed with the reference", bad);
    CHECK(solvable > 200 && unsolvable > 200, "random mix too lopsided (%d solvable, %d not)", solvable, unsolvable);
    printf("  (%d solvable, %d unsolvable)\n", solvable, unsolvable);
}

static void test_invalid_input(void)
{
    section("solver rejects unusable mazes");
    Maze m;
    SolveResult res;
    maze_init(&m);                        /* empty: 0 x 0, no start/exit */
    CHECK(solver_run(&m, ALGO_BFS, &res) == SOLVE_INVALID_MAZE, "empty maze accepted");
    CHECK(res.path == NULL && !res.ran, "result should be empty");

    if (!load(&m, "mazes/small.txt")) { failures++; return; }
    CHECK(solver_run(&m, ALGO_COUNT, &res) == SOLVE_INVALID_MAZE, "bad algorithm accepted");
    m.start_pos.row = 99;                 /* start outside the grid */
    CHECK(solver_run(&m, ALGO_DFS, &res) == SOLVE_INVALID_MAZE, "out-of-range start accepted");
}

static void test_allocation_failure(void)
{
    section("allocation failures (every malloc position, both algorithms)");
    Maze m;
    if (!load(&m, "mazes/small.txt")) { failures++; return; }

    for (int algo = 0; algo < ALGO_COUNT; algo++) {
        int failed_runs = 0;
        int succeeded = 0;
        for (int n = 0; n < 20 && !succeeded; n++) {
            SolveResult res;
            allocs_before_failure = n;
            SolveStatus st = solver_run(&m, (Algorithm)algo, &res);
            allocs_before_failure = -1;

            if (st == SOLVE_OUT_OF_MEMORY) {
                failed_runs++;
                CHECK(res.path == NULL && !res.ran && !res.found, "failed run left state behind");
            } else {
                CHECK(st == SOLVE_OK && path_is_valid(&m, &res), "run after %d allocations invalid", n);
                succeeded = 1;
                solve_result_free(&res);
            }
        }
        CHECK(failed_runs >= 3 && succeeded, "%s: expected several clean failures then success (%d failures)",
              solver_algorithm_name((Algorithm)algo), failed_runs);
    }
    /* Leaks on the failure paths show up when this binary runs under AddressSanitizer. */
}

static void test_save_roundtrip(void)
{
    section("saved solution reloads as the same maze");
    Maze m, back;
    SolveResult res;
    char err[MAZE_ERROR_SIZE];
    const char *path = "build/_roundtrip.txt";

    if (!load(&m, "mazes/sample.txt")) { failures++; return; }
    CHECK(solver_run(&m, ALGO_BFS, &res) == SOLVE_OK && res.found, "solve failed");
    CHECK(maze_save_to_file(path, &m, res.path, res.path_count, 1, err, sizeof err) == SAVE_OK, "save: %s", err);
    CHECK(maze_save_to_file(path, &m, res.path, res.path_count, 0, err, sizeof err) == SAVE_EXISTS,
          "existing file should not be overwritten without permission");
    CHECK(load(&back, path), "reload failed");

    int same = back.rows == m.rows && back.cols == m.cols;
    for (int r = 0; same && r < m.rows; r++)
        for (int c = 0; c < m.cols; c++)
            if (back.cells[r][c] != m.cells[r][c]) same = 0;
    CHECK(same, "reloaded maze differs from the original");
    remove(path);
    solve_result_free(&res);
}

int main(void)
{
    test_simple();
    test_multiple_paths();
    test_backtracking();
    test_no_solution();
    test_adjacent();
    test_large();
    test_repeated();
    test_random();
    test_invalid_input();
    test_allocation_failure();
    test_save_roundtrip();

    printf("\n%d checks, %d failed\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
