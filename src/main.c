#include <stdio.h>

#include "maze.h"
#include "maze_loader.h"
#include "player.h"
#include "validation.h"

#define DEFAULT_MAZE_PATH "mazes/sample.txt"

typedef enum {
    MENU_LOAD = 1,
    MENU_DISPLAY,
    MENU_RESTART,
    MENU_MOVE,
    MENU_EXIT
} MenuChoice;

typedef struct {
    Maze maze;
    Player player;
    int maze_loaded;
} AppState;

static void print_menu(void)
{
    printf("\n=== Maze Solver ===\n");
    printf("1. Load maze\n");
    printf("2. Display maze\n");
    printf("3. Start/restart player\n");
    printf("4. Move player\n");
    printf("5. Exit\n");
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
        printf("You already reached the exit. Use option 3 to restart.\n");
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
            printf("You already reached the exit. Use option 3 to restart.\n");
            return;
        }
    }
}

int main(void)
{
    AppState app;
    char line[INPUT_BUF_SIZE];

    app.maze_loaded = 0;
    maze_init(&app.maze);
    player_reset(&app.player, &app.maze);

    for (;;) {
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
        case MENU_LOAD:    handle_load(&app);    break;
        case MENU_DISPLAY: handle_display(&app); break;
        case MENU_RESTART: handle_restart(&app); break;
        case MENU_MOVE:    handle_move(&app);    break;
        case MENU_EXIT:    printf("Goodbye!\n"); return 0;
        }
    }
    return 0;
}
