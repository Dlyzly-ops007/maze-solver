# Maze Solver

A small console maze program written in C. It loads a maze from a text file, draws it in the terminal, and lets you walk through it with the keyboard.

This is an educational project built in phases. It practises 2D arrays, structs, enums, multi-file organisation, input validation, file handling and simple program state.

**Current status: Phase 1 (manual exploration).** Maze-solving algorithms are planned for Phase 2 and are **not** implemented yet.

## Phase 1 features

- Maze stored as a 2D array of `CellType` values (`CELL_WALL`, `CELL_OPEN`, `CELL_START`, `CELL_EXIT`) inside a `Maze` struct
- Any maze size up to `MAZE_MAX_ROWS` x `MAZE_MAX_COLS` (40 x 60, set in `src/maze.h`)
- Loading from a plain text file, with validation:
  - file exists and can be read
  - only valid characters
  - rectangular grid within the size limits
  - exactly one start and exactly one exit
  - start/exit positions sit on the right cells
- A bad file never replaces the maze you already loaded
- Terminal display with the player (`P`) drawn separately from the start and exit
- W/A/S/D movement with wall and boundary checks, move counter, and a completion message
- Menu-driven interface; invalid menu choices and commands are handled without crashing
- Safe input handling: `fgets` with fixed buffers, overlong lines discarded, no `gets`/`scanf("%s")`
- No dynamic memory, so there is nothing to leak

## Maze file format

One character per cell, one row per line, all rows the same length.

| Char | Meaning |
|------|---------|
| `#`  | wall |
| `.`  | open path |
| `S`  | start (exactly one) |
| `E`  | exit (exactly one) |

Any other character is rejected. Trailing blank lines and Windows (`\r\n`) line endings are accepted. Blank lines between rows are not.

```
#######
#S..#.#
#.#.#.#
#.#...E
#######
```

Sample mazes are in `mazes/`:

- `sample.txt` - 11 x 21
- `small.txt` - 5 x 7, quick to finish

The exit may sit on the border (as above) and the border does not have to be walled. Walking off the edge is simply blocked.

## Controls

| Key | Action |
|-----|--------|
| `W` | move up |
| `A` | move left |
| `S` | move down |
| `D` | move right |
| `Q` | leave movement mode and return to the menu |

Keys work in upper or lower case. Enter one command per line and press Enter.

Menu:

1. Load maze - asks for a file path (press Enter for `mazes/sample.txt`); the player is placed at the start
2. Display maze
3. Start/restart player - move the player back to the start and reset the move count
4. Move player - enter movement mode
5. Exit

## Build

Requires GCC (or any C11 compiler) and, optionally, `make`.

```
make
```

or directly:

```
gcc -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -O2 -o maze_solver src/*.c
```

The build produces no warnings with these flags.

## Run

Run from the project root so the default `mazes/` path works:

```
./maze_solver
```

## Example usage

Lines starting with `>` are typed by the user; the maze redraws after each move (only the last frame is shown here).

```
=== Maze Solver ===
1. Load maze
2. Display maze
3. Start/restart player
4. Move player
5. Exit
Choice: > 1
Maze file path [mazes/sample.txt]: > mazes/small.txt
Loaded 'mazes/small.txt' (5 rows x 7 cols). Player placed at start.

# # # # # # #
# P . . # . #
# . # . # . #
# . # . . . E
# # # # # # #

# wall  . path  S start  E exit  P you
Moves: 0

Choice: > 4
Move with W/A/S/D (one per line). Q returns to the menu.
Move> > w
Blocked: there is a wall there.
Move> > x
Invalid command. Use W, A, S, D to move or Q to go back.
Move> > d
  ...
Move> > d

# # # # # # #
# S . . # . #
# . # . # . #
# . # . . . P

# wall  . path  S start  E exit  P you
Moves: 7
*** You reached the exit in 7 moves! Maze complete. ***
```

## Tests

`tests/run_tests.sh` builds the program and runs scripted sessions against it (also available as `make test`). It covers valid and invalid mazes, missing files, walls, boundaries, reaching the exit, invalid input, and restarting/reloading. Invalid sample files are in `tests/mazes/`.

## Project layout

```
src/main.c          menu loop and user interaction
src/maze.c/.h       Maze struct, cell types, display
src/maze_loader.c/.h  reads a maze file into a Maze (file handling only)
src/player.c/.h     Player struct and movement rules
src/validation.c/.h maze validation and input parsing helpers
mazes/              sample mazes
tests/              test script and invalid-maze fixtures
```

## Current limitations

- Manual play only: no hints, no automatic solving, no path display
- Loading does not check that the exit is reachable from the start, so an unwinnable maze is accepted
- One command per line (no `wasd` strings); the screen is redrawn rather than updated in place
- Size limits are compile-time constants in `src/maze.h`
- Not tested on Windows (line endings in maze files are handled)

## Planned: Phase 2

Maze-solving algorithms (for example DFS and BFS), path display, and shortest-path comparison will be added in Phase 2. Phase 1 keeps the maze, player and loading code separate so they can be reused.

## License

PolyForm Noncommercial License 1.0.0 - see [LICENSE](LICENSE).
