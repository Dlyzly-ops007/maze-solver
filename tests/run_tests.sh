#!/usr/bin/env bash
# End-to-end tests: feed scripted input to the program and check its output.
# Run from anywhere:  bash tests/run_tests.sh   (or: make test)
# MAZE_BIN=path SKIP_BUILD=1 runs the same tests against another build (e.g. a sanitizer build).
cd "$(dirname "$0")/.." || exit 1
if [ -z "$SKIP_BUILD" ]; then make -s || exit 1; fi

BIN=${MAZE_BIN:-./maze_solver}
FIX=tests/mazes
pass=0
fail=0

# check "name" "input for printf" "expected" ["expected" ...]
# An expected value starting with "re:" is an extended regex; otherwise a literal substring.
# Also requires a clean exit (status 0) so crashes, hangs and sanitizer errors are caught.
check() {
    local name="$1" input="$2" out rc expected
    shift 2
    out=$(printf "$input" | timeout 10 "$BIN" 2>&1)
    rc=$?
    if [ "$rc" -ne 0 ]; then
        echo "FAIL: $name (exit status $rc)"; fail=$((fail + 1)); return
    fi
    for expected in "$@"; do
        if [[ "$expected" == re:* ]]; then
            grep -qE -- "${expected#re:}" <<< "$out" && continue
        else
            grep -qF -- "$expected" <<< "$out" && continue
        fi
        echo "FAIL: $name (missing: $expected)"; fail=$((fail + 1)); return
    done
    echo "PASS: $name"; pass=$((pass + 1))
}

# check_file "name" file "expected content (printf format)"
check_file() {
    if [ -f "$2" ] && [ "$(cat "$2")" == "$(printf "$3")" ]; then
        echo "PASS: $1"; pass=$((pass + 1))
    else
        echo "FAIL: $1 (file $2 differs or is missing)"; fail=$((fail + 1))
    fi
}

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"; rm -f mazes/small_solved.txt' EXIT
rm -f mazes/small_solved.txt
orig_small=$(sha256sum mazes/small.txt)
orig_sample=$(sha256sum mazes/sample.txt)

# Generated edge cases: too wide, too tall, overlong line, NUL byte
{ printf 'S'; printf '.%.0s' $(seq 1 60); printf 'E\n'; } > "$tmp/too_wide.txt"
{ for i in $(seq 1 41); do printf '#...#\n'; done; } > "$tmp/too_tall.txt"
{ printf 'S'; printf '.%.0s' $(seq 1 400); printf 'E\n'; } > "$tmp/too_long_line.txt"
printf '#####\n#S\0.E#\n#####\n' > "$tmp/nul.txt"

SMALL_SOLVED='#######\n#S**#.#\n#.#*#.#\n#.#***E\n#######'

echo "== Phase 1: loading =="
check "valid maze loads"            '1\nmazes/sample.txt\n9\n'  "Loaded 'mazes/sample.txt' (11 rows x 21 cols)" "Goodbye!"
check "default path on empty input" '1\n\n9\n'                   "Loaded 'mazes/sample.txt'"
check "second sample loads"         '1\nmazes/small.txt\n9\n'   "(5 rows x 7 cols)"
check "CRLF file loads"             "1\n$FIX/crlf.txt\n9\n"      "(3 rows x 5 cols)"
check "missing file"                '1\nmazes/nope.txt\n2\n9\n' "cannot open 'mazes/nope.txt'" "No maze loaded"
check "directory as file"           '1\nmazes\n9\n'             "Could not load maze"
check "invalid: no exit"            "1\n$FIX/no_exit.txt\n9\n"     "found 0" "exit 'E'"
check "invalid: no start"           "1\n$FIX/no_start.txt\n9\n"    "found 0" "start 'S'"
check "invalid: two starts"         "1\n$FIX/two_starts.txt\n9\n"  "found 2" "start 'S'"
check "invalid: two exits"          "1\n$FIX/two_exits.txt\n9\n"   "found 2" "exit 'E'"
check "invalid: ragged rows"        "1\n$FIX/ragged.txt\n9\n"      "line 3 has 4 columns, expected 5"
check "invalid: bad character"      "1\n$FIX/bad_char.txt\n9\n"    "line 2, column 4: invalid character 'X'"
check "invalid: blank inside"       "1\n$FIX/blank_inside.txt\n9\n" "unexpected blank line"
check "invalid: empty file"         "1\n$FIX/empty.txt\n9\n"       "maze is empty"
check "invalid: too wide"           "1\n$tmp/too_wide.txt\n9\n"    "wider than the maximum"
check "invalid: too tall"           "1\n$tmp/too_tall.txt\n9\n"    "too many rows"
check "invalid: overlong line"      "1\n$tmp/too_long_line.txt\n9\n" "is too long"
check "invalid: NUL in a line"      "1\n$tmp/nul.txt\n9\n"         "is too long (or contains a NUL character)"
check "overlong path input"         "1\n$(printf 'x%.0s' $(seq 1 600))\n9\n" "Could not load maze"

echo "== Phase 1: display, state, manual play =="
check "display without maze"        '2\n9\n'  "No maze loaded"
check "manual without maze"         '3\n9\n'  "No maze loaded"
check "restart without maze"        '8\n9\n'  "No maze loaded"
check "player shown at start"       '1\nmazes/small.txt\n9\n'  "# P . . # . #"
check "move into wall"              '1\nmazes/small.txt\n3\nw\nq\n9\n'  "Blocked: there is a wall there." "Moves: 0"
check "move outside boundary"       "1\n$FIX/open_border.txt\n3\nw\na\nq\n9\n" "Blocked: that would leave the maze."
check "valid move updates position" '1\nmazes/small.txt\n3\nd\nq\n9\n'  "# S P . # . #" "Moves: 1"
check "lowercase and uppercase"     '1\nmazes/small.txt\n3\nD\nd\nq\n9\n' "Moves: 2"
check "reaching the exit"           '1\nmazes/small.txt\n3\nd\nd\ns\ns\nd\nd\nd\n9\n' "You reached the exit in 7 moves" "# . # . . . P"
check "manual after finishing"      '1\nmazes/small.txt\n3\nd\nd\ns\ns\nd\nd\nd\n3\n9\n' "You already reached the exit"
check "invalid move commands"       '1\nmazes/small.txt\n3\nx\n\nwasd\n12\n  \nq\n9\n' "Invalid command. Use W, A, S, D"
check "invalid menu choices"        'abc\n0\n\n12\n-1\n9\n'  "Invalid choice. Enter a number from 1 to 9." "Goodbye!"
check "very long menu line"         "$(printf 'z%.0s' $(seq 1 1000))\n9\n" "Invalid choice" "Goodbye!"
check "very long move line"         "1\nmazes/small.txt\n3\n$(printf 'w%.0s' $(seq 1 1000))\nq\n9\n" "Invalid command"
check "EOF at menu"                 ''  "Choice:"
check "EOF in manual mode"          '1\nmazes/small.txt\n3\n'  "Move>"
check "EOF at path prompt"          '1\n'  "Maze file path"
check "restart resets player"       '1\nmazes/small.txt\n3\nd\nd\nq\n8\n9\n'  "Player moved back to start." "Moves: 0"
check "restart after finishing"     '1\nmazes/small.txt\n3\nd\nd\ns\ns\nd\nd\nd\n8\n3\nd\nq\n9\n' "Player moved back to start." "Moves: 1"
check "load another maze"           '1\nmazes/small.txt\n3\nd\nq\n1\nmazes/sample.txt\n9\n' "(11 rows x 21 cols)" "Moves: 0"
check "bad load keeps old maze"     "1\nmazes/small.txt\n1\n$FIX/no_exit.txt\n2\n9\n" "Could not load maze" "# P . . # . #"

echo "== Phase 2: solving =="
check "solve without maze"          '4\n5\n6\n7\n9\n'  "No maze loaded"
check "DFS solves simple maze"      '1\nmazes/small.txt\n4\n9\n'  "DFS: path found." "Path length: 7 moves" "Cells explored: 10" "# S * * # . #" "# . # * * * E"
check "BFS solves simple maze"      '1\nmazes/small.txt\n5\n9\n'  "BFS: path found." "Path length: 7 moves" "Cells explored: 11" "# S * * # . #"
check "start and exit stay visible" '1\nmazes/small.txt\n5\n9\n'  "# S * * # . #" "# . # * * * E"
check "BFS gives the shortest path" "1\nmazes/two_routes.txt\n5\n9\n"  "Path length: 10 moves"
check "DFS result kept separate"    "1\nmazes/two_routes.txt\n4\n9\n"  "Path length: 14 moves"
check "DFS with backtracking"       "1\n$FIX/backtrack.txt\n4\n9\n"   "DFS: path found." "Path length: 4 moves" "Cells explored: 6"
check "start next to exit"          "1\n$FIX/adjacent.txt\n4\n5\n9\n" "Path length: 1 moves" "Cells explored: 2"
check "medium maze"                 '1\nmazes/sample.txt\n4\n5\n6\n9\n' "re:DFS +yes +63 +64" "re:BFS +yes +63 +84"
check "larger maze (39 x 59)"       '1\nmazes/large.txt\n4\n5\n6\n9\n' "re:DFS +yes +1100 +1103" "re:BFS +yes +972 +1103"
check "no solution: DFS and BFS"    "1\n$FIX/unsolvable.txt\n4\n5\n6\n9\n" "DFS: no path from start to exit." "BFS: no path from start to exit." "re:DFS +no +- +10" "re:BFS +no +- +10"
check "no solution: nothing to save" "1\n$FIX/unsolvable.txt\n4\n7\n9\n" "No solved maze to save"

echo "== Phase 2: statistics =="
check "stats before solving"        '1\nmazes/small.txt\n6\n9\n'  "No solver has been run yet"
check "stats: DFS and BFS separate" "1\nmazes/two_routes.txt\n4\n5\n6\n9\n" "re:DFS +yes +14 +15" "re:BFS +yes +10 +21" "BFS path length (shortest possible in this maze): 10 moves"
check "stats: one algorithm run"    "1\nmazes/two_routes.txt\n4\n6\n9\n" "re:DFS +yes +14 +15" "re:BFS +- +- +not run"
check "stats show latest solution"  '1\nmazes/small.txt\n5\n6\n9\n' "Most recent solution (BFS):"
check "repeated solving"            '1\nmazes/small.txt\n4\n4\n5\n5\n4\n6\n9\n' "re:DFS +yes +7 +10" "re:BFS +yes +7 +11"
check "solving leaves maze intact"  '1\nmazes/small.txt\n5\n2\n9\n' "# P . . # . #"
check "manual play after solving"   '1\nmazes/small.txt\n5\n3\nd\nq\n9\n' "# S P . # . #" "Moves: 1"
check "new maze clears statistics"  '1\nmazes/small.txt\n5\n1\nmazes/sample.txt\n6\n9\n' "Loaded 'mazes/sample.txt'" "No solver has been run yet"
check "failed load keeps results"   "1\nmazes/small.txt\n5\n1\n$FIX/no_exit.txt\n6\n9\n" "Could not load maze" "re:BFS +yes +7 +11"
check "solve, load, solve again"    '1\nmazes/small.txt\n5\n1\nmazes/sample.txt\n5\n9\n' "Path length: 63 moves"

echo "== Phase 2: saving =="
check "save without solving"        '1\nmazes/small.txt\n7\n9\n'  "No solved maze to save"
check "save writes the solution"    "1\nmazes/small.txt\n5\n7\n$tmp/out.txt\n9\n"  "Saved BFS solution to '$tmp/out.txt'."
check_file "saved file content"     "$tmp/out.txt" "$SMALL_SOLVED"
check "saved solution reloads"      "1\n$tmp/out.txt\n2\n9\n"  "(5 rows x 7 cols)" "# P . . # . #"
check "no overwrite without yes"    "1\nmazes/small.txt\n4\n7\n$tmp/out.txt\nn\n9\n"  "already exists" "Not saved."
check "EOF at overwrite prompt"     "1\nmazes/small.txt\n4\n7\n$tmp/out.txt\n"  "Not saved."
printf 'old contents\n' > "$tmp/existing.txt"
check "overwrite when confirmed"    "1\nmazes/small.txt\n4\n7\n$tmp/existing.txt\ny\n9\n"  "Saved DFS solution"
check_file "overwritten file content" "$tmp/existing.txt" "$SMALL_SOLVED"
check "refuse to overwrite original" '1\nmazes/small.txt\n5\n7\nmazes/small.txt\n9\n'  "Refusing to overwrite the original maze file"
check "unwritable save path"        '1\nmazes/small.txt\n5\n7\n/nonexistent_dir/x.txt\n9\n'  "Could not save: cannot write"
check "save uses latest solution"   "1\nmazes/two_routes.txt\n4\n5\n7\n$tmp/latest.txt\n9\n"  "Saved BFS solution"
check "default save name"           '1\nmazes/small.txt\n5\n7\n\n9\n'  "Saved BFS solution to 'mazes/small_solved.txt'."
check_file "default-named file"     mazes/small_solved.txt "$SMALL_SOLVED"
check "EOF at save prompt"          '1\nmazes/small.txt\n5\n7\n'  "Save BFS solution to"
check "original maze files intact"  '1\nmazes/small.txt\n9\n' "Loaded"
if [ "$(sha256sum mazes/small.txt)" == "$orig_small" ] && [ "$(sha256sum mazes/sample.txt)" == "$orig_sample" ]; then
    echo "PASS: original maze files unchanged after all tests"; pass=$((pass + 1))
else
    echo "FAIL: an original maze file was modified"; fail=$((fail + 1))
fi

echo
echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
