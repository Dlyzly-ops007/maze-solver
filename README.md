# Maze Solver

A console maze program written in C. It loads a maze from a text file, lets you walk through it by hand, and can solve it automatically with **depth-first search (DFS)** or **breadth-first search (BFS)**, then draw and save the solution.

It is an educational project built in phases, practising 2D arrays, structs, enums, dynamic memory, multi-file organisation, input validation, file handling and basic graph traversal.

- **Phase 1** - load, validate, display and manually explore a maze
- **Phase 2** (this version) - DFS and BFS solving, path reconstruction, solver statistics, saving solved mazes

## Features

### Phase 1 (unchanged)

- Maze stored as a 2D array of `CellType` values (`CELL_WALL`, `CELL_OPEN`, `CELL_START`, `CELL_EXIT`) inside a `Maze` struct, any size up to 40 x 60 (`MAZE_MAX_ROWS` / `MAZE_MAX_COLS` in `src/maze.h`)
- Loading with validation: file readable, valid characters, rectangular grid within the size limits, exactly one start and one exit, start/exit on the right cells
- A bad file never replaces the maze you already loaded
- W/A/S/D manual exploration with wall and boundary checks, move counter and a completion message
- Safe input handling: `fgets` with fixed buffers, overlong lines discarded, invalid commands rejected

### Phase 2

- **DFS and BFS solvers** that only walk through open cells, never cross walls, never leave the grid, and correctly report when the exit is unreachable
- **Path reconstruction**: after a successful search the route from start to exit is rebuilt and drawn with `*`. The maze itself is never modified; the solution is a separate list of cells drawn as an overlay, and `S` / `E` stay visible
- **Solver statistics** for each algorithm: whether a path was found, path length, cells explored, and which algorithm produced it. DFS and BFS results are stored separately
- **Save solved maze** to a new file (never over the original)
- **Load another maze at any time** without restarting; old solutions are cleared
- Dynamic memory with checked allocations and cleanup on every path, including allocation failure

## Maze file format

One character per cell, one row per line, all rows the same length.

| Char | Meaning |
|------|---------|
| `#`  | wall |
| `.`  | open path |
| `S`  | start (exactly one) |
| `E`  | exit (exactly one) |
| `*`  | solution marker, only found in saved solutions; loaded as an open cell |

Any other character is rejected. Trailing blank lines and Windows (`\r\n`) line endings are accepted; blank lines between rows are not. The border does not have to be walled, and the exit may sit on the border.

```
#######
#S..#.#
#.#.#.#
#.#...E
#######
```

Because `*` loads as an open cell, a saved solution can be loaded again and solved again; the original path drawing is not kept. Mazes with no route from `S` to `E` load fine and are reported as unsolvable when you try to solve them.

Included mazes in `mazes/`:

| File | Size | Notes |
|------|------|-------|
| `small.txt` | 5 x 7 | one route |
| `two_routes.txt` | 7 x 9 | two routes of different length, good for comparing DFS and BFS |
| `sample.txt` | 11 x 21 | winding, one route |
| `large.txt` | 39 x 59 | long corridor with two shortcuts |

## Menu and controls

```
1. Load maze
2. Display maze
3. Manual exploration
4. Solve with DFS
5. Solve with BFS
6. Show solver statistics
7. Save solved maze
8. Restart player
9. Exit
```

Manual exploration keys (one per line, either case): `W` up, `A` left, `S` down, `D` right, `Q` back to the menu.

- **1** asks for a path (Enter = `mazes/sample.txt`). Loading also places the player at the start and clears earlier solver results.
- **4 / 5** run a solver, draw the solution and print a summary. Solvers work on the maze only; they do not move the manual-play player.
- **6** prints a table for both algorithms and redraws the most recent solution.
- **7** saves the most recent *successful* solution. The default file name is the maze name plus `_solved` (for example `mazes/small_solved.txt`). If the file exists you are asked before overwriting it, and saving over the file the maze was loaded from is refused.
- **8** moves the manual-play player back to the start.

Note: menu numbers changed from Phase 1 to make room for the solver options.

## How the algorithms work

Both algorithms treat the maze as a graph: every non-wall cell is a node, and each cell is connected to its open up/right/down/left neighbours. Both try neighbours in the same fixed order (up, right, down, left), remember each cell's parent, and stop when the exit is reached or every reachable cell has been visited.

### DFS (depth-first search)

DFS follows one direction as far as it can. When a cell has no unvisited neighbours it backtracks to the previous cell and tries that cell's next direction. It is implemented iteratively with an explicit stack (no recursion), so large mazes cannot overflow the call stack. The path it finds depends on the neighbour order and is not guaranteed to be the shortest.

### BFS (breadth-first search)

BFS explores outward in rings: all cells one move from the start, then two moves, and so on, using a queue. In a maze where every move costs the same, the first time BFS reaches the exit is by a shortest route, so the **BFS path length is the minimum number of moves** for that maze. DFS is run separately as written and is not adjusted to imitate BFS.

### Path reconstruction

Each visited cell records the cell it was reached from. When the exit is found, the program follows those links from the exit back to the start, counting cells, allocates an array of exactly that size, and fills it in start-to-exit order. That array is what gets drawn and saved.

### Solver statistics

| Value | Meaning |
|-------|---------|
| Found | whether the exit is reachable |
| Path length | moves from start to exit (number of path cells minus one) |
| Cells explored | cells the search visited before stopping, including the start. DFS counts a cell when it steps onto it; BFS counts a cell when it takes it off the queue. Both include the exit when it is found |

If no path exists, both algorithms end up exploring every cell reachable from the start. The numbers are simply what was measured for that maze; they say nothing general about which algorithm is preferable. For example, on `two_routes.txt` BFS finds a shorter path but explores more cells than DFS, while on other mazes the two can be equal.

## Build

Requires GCC (or another C11 compiler) and optionally `make`.

```
make
```

or directly:

```
gcc -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -O2 -o maze_solver src/*.c
```

## Run

From the project root, so the default `mazes/` path works:

```
./maze_solver
```

## Example usage

`>` marks what the user types. Menu reprints are omitted.

```
Choice: > 1
Maze file path [mazes/sample.txt]: > mazes/two_routes.txt
Loaded 'mazes/two_routes.txt' (7 rows x 9 cols). Player placed at start.

Choice: > 4

# # # # # # # # #
# S * * # * * * #
# . # * # * # * #
# . # * * * # * #
# . # # # # # * #
# . . . . . . E #
# # # # # # # # #

# wall  . open  S start  E exit  * solution
DFS: path found.
  Path length: 14 moves
  Cells explored: 15
```

Then BFS on the same maze, the statistics table, and saving:

```
Choice: > 5

# # # # # # # # #
# S . . # . . . #
# * # . # . # . #
# * # . . . # . #
# * # # # # # . #
# * * * * * * E #
# # # # # # # # #

# wall  . open  S start  E exit  * solution
BFS: path found.
  Path length: 10 moves
  Cells explored: 21

Choice: > 6
Solver statistics for 'mazes/two_routes.txt'
Algorithm  Found  Path length  Cells explored
DFS        yes    14           15
BFS        yes    10           21
Path length = moves from start to exit. Cells explored = cells visited before the search stopped.
BFS path length (shortest possible in this maze): 10 moves

Choice: > 7
Save BFS solution to [mazes/two_routes_solved.txt]: >
Saved BFS solution to 'mazes/two_routes_solved.txt'.
```

The saved file:

```
#########
#S..#...#
#*#.#.#.#
#*#...#.#
#*#####.#
#******E#
#########
```

## How Phase 1 and Phase 2 fit together

Phase 2 adds to Phase 1 rather than replacing it:

- `maze.c/.h` (maze representation, display) and `maze_loader.c/.h` (file reading and validation calls) are the Phase 1 modules. Phase 2 added the path overlay (`maze_display_path`), file saving, and `*` handling.
- `player.c/.h` (manual movement) is unchanged and is reached through menu option 3.
- `validation.c/.h` still validates every loaded maze; solved mazes go through the same loader when reloaded.
- `solver.c/.h` is new. It reads a `Maze` and returns a `SolveResult`; it never touches the player or the file system. Path reconstruction lives there too.
- `main.c` ties it together: the loaded maze, the player, and one stored `SolveResult` per algorithm.

```
src/main.c            menu loop, statistics table, save prompts
src/maze.c/.h         Maze struct, cell types, display, path overlay
src/maze_loader.c/.h  reading and saving maze files
src/player.c/.h       manual movement rules
src/solver.c/.h       DFS, BFS, path reconstruction, statistics data
src/validation.c/.h   maze validation and input parsing helpers
mazes/                sample mazes
tests/                unit tests, end-to-end tests, test mazes
```

## Tests

```
make test
```

runs two suites:

- `tests/test_solver.c` - unit tests of the solver: path validity (starts at `S`, ends at `E`, single orthogonal steps, no walls, no repeated cells), BFS length compared with an independent shortest-distance calculation on 3000 random mazes, DFS/BFS agreement on solvable and unsolvable mazes, repeated runs, and simulated `malloc` failure at every allocation point (uses GNU ld `--wrap`)
- `tests/run_tests.sh` - scripted sessions against the real program: loading, invalid mazes, walls and boundaries, manual play, DFS, BFS, backtracking, unsolvable mazes, start next to exit, the 39 x 59 maze, repeated solving, loading a new maze after solving, and saving (including not overwriting the original)

Both suites also pass under AddressSanitizer and UBSan.

## Known limitations

- Moves are up/down/left/right only, every move costs the same; there are no weights or diagonals
- DFS results depend on the fixed neighbour order (up, right, down, left) and are not necessarily shortest; explored-cell counts are defined as above and are not a timing measurement
- Only the most recent successful solution can be saved, and one result is kept per algorithm
- Phase 1 has no maze editor or creation feature, so there is nothing to save except solved mazes
- Saved solutions reload with the path marks treated as ordinary open cells
- Maximum size is 40 x 60, set at compile time in `src/maze.h`
- Overwrite protection for the original file compares the typed path text; any existing file also needs a `y` confirmation
- The screen is redrawn rather than updated in place
- Developed and tested with GCC on Linux; the unit tests need GNU ld

## License

PolyForm Noncommercial License 1.0.0 - see [LICENSE](LICENSE).
