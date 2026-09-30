#include <stdio.h>
#include <string.h>

#include "maze.h"
#include "maze_loader.h"
#include "player.h"
#include "solver.h"
#include "validation.h"

#define DEFAULT_MAZE_PATH  "mazes/sample.txt"
#define FALLBACK_SAVE_PATH "solved_maze.txt"

typedef enum {
    MENU_LOAD = 1,
    MENU_DISPLAY,
    MENU_MANUAL,
    MENU_SOLVE_DFS,
    MENU_SOLVE_BFS,
    MENU_STATS,
    MENU_SAVE,
    MENU_RESTART,
    MENU_EXIT
} MenuChoice;

typedef struct {
    Maze maze;
    Player player;
    int maze_loaded;
    char maze_path[INPUT_BUF_SIZE];      /* file the current maze came from */
    SolveResult results[ALGO_COUNT];     /* latest result per algorithm, kept separate */
    int last_solved;                     /* Algorithm of the latest successful solve, or -1 */
} AppState;

static void print_menu(void)
{
    printf("\n=== Maze Solver ===\n");
    printf("1. Load maze\n");
    printf("2. Display maze\n");
    printf("3. Manual exploration\n");
    printf("4. Solve with DFS\n");
    printf("5. Solve with BFS\n");
    printf("6. Show solver statistics\n");
    printf("7. Save solved maze\n");
    printf("8. Restart player\n");
    printf("9. Exit\n");
    printf("Choice: ");
}

static int require_maze(const AppState *app)
{
    if (!app->maze_loaded) {
        printf("No maze loaded. Choose option 1 first.\n");
        return 0;
    }
    return 1;
}

static void show_state(const AppState *app)
{
    maze_display(&app->maze, &app->player.pos);
    printf("Moves: %d\n", app->player.moves);
}

/* Frees every stored solution. Called when a new maze replaces the old one
 * and before the program exits. */
static void clear_results(AppState *app)
{
    for (int i = 0; i < ALGO_COUNT; i++) {
        solve_result_free(&app->results[i]);
    }
    app->last_solved = -1;
}

/* ---------- Phase 1: loading and manual play ---------- */

static void handle_load(AppState *app)
{
    char path[INPUT_BUF_SIZE];
    char err[MAZE_ERROR_SIZE];
    Maze loaded;

    printf("Maze file path [%s]: ", DEFAULT_MAZE_PATH);
    if (!input_read_line(path, sizeof path)) {
        return;
    }
    if (path[0] == '\0') {
        snprintf(path, sizeof path, "%s", DEFAULT_MAZE_PATH);
    }

    /* Load into a temporary so a bad file never replaces a working maze. */
    if (!maze_load_from_file(path, &loaded, err, sizeof err)) {
        printf("Could not load maze: %s\n", err);
        return;
    }

    app->maze = loaded;
    app->maze_loaded = 1;
    snprintf(app->maze_path, sizeof app->maze_path, "%s", path);
    clear_results(app);   /* solutions belong to the previous maze */
    player_reset(&app->player, &app->maze);
    printf("Loaded '%s' (%d rows x %d cols). Player placed at start.\n",
           path, app->maze.rows, app->maze.cols);
    show_state(app);
}

static void handle_display(const AppState *app)
{
    if (require_maze(app)) {
        show_state(app);
    }
}

static void handle_restart(AppState *app)
{
    if (require_maze(app)) {
        player_reset(&app->player, &app->maze);
        printf("Player moved back to start.\n");
        show_state(app);
    }
}

static void handle_move(AppState *app)
{
    char line[INPUT_BUF_SIZE];
    Direction dir;

    if (!require_maze(app)) {
        return;
    }
    if (app->player.finished) {
        printf("You already reached the exit. Use option 8 to restart.\n");
        return;
    }

    printf("Move with W/A/S/D (one per line). Q returns to the menu.\n");
    show_state(app);

    for (;;) {
        printf("Move> ");
        if (!input_read_line(line, sizeof line)) {
            return;
        }
        if (input_is_quit(line)) {
            return;
        }
        if (!input_parse_direction(line, &dir)) {
            printf("Invalid command. Use W, A, S, D to move or Q to go back.\n");
            continue;
        }

        switch (player_move(&app->player, &app->maze, dir)) {
        case MOVE_OK:
            show_state(app);
            break;
        case MOVE_BLOCKED_WALL:
            printf("Blocked: there is a wall there.\n");
            break;
        case MOVE_OUT_OF_BOUNDS:
            printf("Blocked: that would leave the maze.\n");
            break;
        case MOVE_REACHED_EXIT:
            show_state(app);
            printf("*** You reached the exit in %d moves! Maze complete. ***\n",
                   app->player.moves);
            return;
        case MOVE_ALREADY_FINISHED:
            printf("You already reached the exit. Use option 8 to restart.\n");
            return;
        }
    }
}

/* ---------- Phase 2: solving, statistics, saving ---------- */

static void handle_solve(AppState *app, Algorithm algo)
{
    SolveResult result;
    const char *name = solver_algorithm_name(algo);

    if (!require_maze(app)) {
        return;
    }

    switch (solver_run(&app->maze, algo, &result)) {
    case SOLVE_OK:
        break;
    case SOLVE_OUT_OF_MEMORY:
        printf("%s: out of memory while solving. Nothing was changed.\n", name);
        return;
    case SOLVE_INVALID_MAZE:
        printf("%s: the loaded maze is not valid for solving.\n", name);
        return;
    }

    /* Replace only this algorithm's previous result; the other one is untouched. */
    solve_result_free(&app->results[algo]);
    app->results[algo] = result;

    if (result.found) {
        app->last_solved = (int)algo;
        maze_display_path(&app->maze, result.path, result.path_count);
        printf("%s: path found.\n", name);
        printf("  Path length: %d moves\n", result.path_length);
    } else {
        printf("%s: no path from start to exit.\n", name);
    }
    printf("  Cells explored: %d\n", result.explored);
}

static void handle_stats(const AppState *app)
{
    int any = 0;

    if (!require_maze(app)) {
        return;
    }
    for (int i = 0; i < ALGO_COUNT; i++) {
        any = any || app->results[i].ran;
    }
    if (!any) {
        printf("No solver has been run yet. Use option 4 or 5.\n");
        return;
    }

    printf("\nSolver statistics for '%s'\n", app->maze_path);
    printf("%-10s %-6s %-12s %s\n", "Algorithm", "Found", "Path length", "Cells explored");
    for (int i = 0; i < ALGO_COUNT; i++) {
        const SolveResult *r = &app->results[i];
        const char *name = solver_algorithm_name((Algorithm)i);
        if (!r->ran) {
            printf("%-10s %-6s %-12s %s\n", name, "-", "-", "not run");
        } else if (r->found) {
            printf("%-10s %-6s %-12d %d\n", name, "yes", r->path_length, r->explored);
        } else {
            printf("%-10s %-6s %-12s %d\n", name, "no", "-", r->explored);
        }
    }
    printf("Path length = moves from start to exit. Cells explored = cells visited before the search stopped.\n");

    const SolveResult *bfs = &app->results[ALGO_BFS];
    if (bfs->ran && bfs->found) {
        printf("BFS path length (shortest possible in this maze): %d moves\n", bfs->path_length);
    }

    if (app->last_solved >= 0) {
        const SolveResult *last = &app->results[app->last_solved];
        printf("\nMost recent solution (%s):\n", solver_algorithm_name(last->algorithm));
        maze_display_path(&app->maze, last->path, last->path_count);
    }
}

/* "mazes/small.txt" -> "mazes/small_solved.txt". Falls back to a fixed name if
 * the result would not fit. */
static void default_save_path(const char *source, char *out, size_t size)
{
    const char *slash = strrchr(source, '/');
    const char *dot = strrchr(source, '.');
    int written;

    if (dot != NULL && (slash == NULL || dot > slash)) {
        written = snprintf(out, size, "%.*s_solved%s", (int)(dot - source), source, dot);
    } else {
        written = snprintf(out, size, "%s_solved.txt", source);
    }
    if (written < 0 || (size_t)written >= size) {
        snprintf(out, size, "%s", FALLBACK_SAVE_PATH);
    }
}

static void handle_save(const AppState *app)
{
    char default_path[INPUT_BUF_SIZE];
    char path[INPUT_BUF_SIZE];
    char answer[INPUT_BUF_SIZE];
    char err[MAZE_ERROR_SIZE];

    if (!require_maze(app)) {
        return;
    }
    if (app->last_solved < 0) {
        printf("No solved maze to save. Run a solver that finds a path first (option 4 or 5).\n");
        return;
    }

    const SolveResult *solution = &app->results[app->last_solved];
    default_save_path(app->maze_path, default_path, sizeof default_path);

    printf("Save %s solution to [%s]: ", solver_algorithm_name(solution->algorithm), default_path);
    if (!input_read_line(path, sizeof path)) {
        return;
    }
    if (path[0] == '\0') {
        snprintf(path, sizeof path, "%s", default_path);
    }

    if (strcmp(path, app->maze_path) == 0) {
        printf("Refusing to overwrite the original maze file '%s'. Choose a different name.\n", path);
        return;
    }

    SaveStatus status = maze_save_to_file(path, &app->maze, solution->path,
                                          solution->path_count, 0, err, sizeof err);
    if (status == SAVE_EXISTS) {
        printf("'%s' already exists. Overwrite? (y/N): ", path);
        if (!input_read_line(answer, sizeof answer) || !input_is_yes(answer)) {
            printf("Not saved.\n");
            return;
        }
        status = maze_save_to_file(path, &app->maze, solution->path,
                                   solution->path_count, 1, err, sizeof err);
    }

    if (status == SAVE_OK) {
        printf("Saved %s solution to '%s'.\n", solver_algorithm_name(solution->algorithm), path);
    } else {
        printf("Could not save: %s\n", err);
    }
}

int main(void)
{
    AppState app;
    char line[INPUT_BUF_SIZE];
    int running = 1;

    app.maze_loaded = 0;
    app.maze_path[0] = '\0';
    maze_init(&app.maze);
    player_reset(&app.player, &app.maze);
    for (int i = 0; i < ALGO_COUNT; i++) {
        solve_result_init(&app.results[i]);
    }
    app.last_solved = -1;

    while (running) {
        print_menu();
        if (!input_read_line(line, sizeof line)) {
            printf("\n");
            break;
        }

        int choice = input_parse_menu_choice(line, MENU_EXIT);
        if (choice == 0) {
            printf("Invalid choice. Enter a number from 1 to %d.\n", MENU_EXIT);
            continue;
        }

        switch ((MenuChoice)choice) {
        case MENU_LOAD:      handle_load(&app);                 break;
        case MENU_DISPLAY:   handle_display(&app);              break;
        case MENU_MANUAL:    handle_move(&app);                 break;
        case MENU_SOLVE_DFS: handle_solve(&app, ALGO_DFS);      break;
        case MENU_SOLVE_BFS: handle_solve(&app, ALGO_BFS);      break;
        case MENU_STATS:     handle_stats(&app);                break;
        case MENU_SAVE:      handle_save(&app);                 break;
        case MENU_RESTART:   handle_restart(&app);              break;
        case MENU_EXIT:      printf("Goodbye!\n"); running = 0; break;
        }
    }

    clear_results(&app);
    return 0;
}
