#!/usr/bin/env bash
# End-to-end tests: feed scripted input to the program and check its output.
# Run from anywhere:  bash tests/run_tests.sh   (or: make test)
cd "$(dirname "$0")/.." || exit 1
make -s || exit 1

BIN=./maze_solver
FIX=tests/mazes
pass=0
fail=0

# check "name" "input for printf" "expected text" ["more expected text" ...]
# Also requires a clean exit (status 0) so crashes and hangs are caught.
check() {
    local name="$1" input="$2" out rc
    shift 2
    out=$(printf "$input" | timeout 5 "$BIN" 2>&1)
    rc=$?
    if [ "$rc" -ne 0 ]; then
        echo "FAIL: $name (exit status $rc)"; fail=$((fail + 1)); return
    fi
    for expected in "$@"; do
        if ! grep -qF -- "$expected" <<< "$out"; then
            echo "FAIL: $name (missing: $expected)"; fail=$((fail + 1)); return
        fi
    done
    echo "PASS: $name"; pass=$((pass + 1))
}

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

# Generated edge cases: too wide, too tall, overlong line
{ printf 'S'; printf '.%.0s' $(seq 1 60); printf 'E\n'; } > "$tmp/too_wide.txt"
{ for i in $(seq 1 41); do printf '#...#\n'; done; } > "$tmp/too_tall.txt"
{ printf 'S'; printf '.%.0s' $(seq 1 400); printf 'E\n'; } > "$tmp/too_long_line.txt"

# --- loading ---
check "valid maze loads"            '1\nmazes/sample.txt\n5\n'  "Loaded 'mazes/sample.txt' (11 rows x 21 cols)" "Goodbye!"
check "default path on empty input" '1\n\n5\n'                   "Loaded 'mazes/sample.txt'"
check "second sample loads"         '1\nmazes/small.txt\n5\n'   "(5 rows x 7 cols)"
check "CRLF file loads"             "1\n$FIX/crlf.txt\n5\n"      "(3 rows x 5 cols)"

check "missing file"                '1\nmazes/nope.txt\n2\n5\n' "cannot open 'mazes/nope.txt'" "No maze loaded"
check "directory as file"           '1\nmazes\n5\n'             "Could not load maze"
check "invalid: no exit"            "1\n$FIX/no_exit.txt\n5\n"     "found 0" "exit 'E'"
check "invalid: no start"           "1\n$FIX/no_start.txt\n5\n"    "found 0" "start 'S'"
check "invalid: two starts"         "1\n$FIX/two_starts.txt\n5\n"  "found 2" "start 'S'"
check "invalid: two exits"          "1\n$FIX/two_exits.txt\n5\n"   "found 2" "exit 'E'"
check "invalid: ragged rows"        "1\n$FIX/ragged.txt\n5\n"      "line 3 has 4 columns, expected 5"
check "invalid: bad character"      "1\n$FIX/bad_char.txt\n5\n"    "line 2, column 4: invalid character 'X'"
check "invalid: blank inside"       "1\n$FIX/blank_inside.txt\n5\n" "unexpected blank line"
check "invalid: empty file"         "1\n$FIX/empty.txt\n5\n"       "maze is empty"
check "invalid: too wide"           "1\n$tmp/too_wide.txt\n5\n"    "wider than the maximum"
check "invalid: too tall"           "1\n$tmp/too_tall.txt\n5\n"    "too many rows"
check "invalid: overlong line"      "1\n$tmp/too_long_line.txt\n5\n" "is too long"
check "overlong path input"         "1\n$(printf 'x%.0s' $(seq 1 600))\n5\n" "Could not load maze"

# --- display / state ---
check "display without maze"        '2\n5\n'  "No maze loaded"
check "move without maze"           '4\n5\n'  "No maze loaded"
check "restart without maze"        '3\n5\n'  "No maze loaded"
check "player shown at start"       '1\nmazes/small.txt\n5\n'  "# P . . # . #"

# --- movement ---
check "move into wall"              '1\nmazes/small.txt\n4\nw\nq\n5\n'  "Blocked: there is a wall there." "Moves: 0"
check "move outside boundary"       "1\n$FIX/open_border.txt\n4\nw\na\nq\n5\n" "Blocked: that would leave the maze."
check "valid move updates position" '1\nmazes/small.txt\n4\nd\nq\n5\n'  "# S P . # . #" "Moves: 1"
check "lowercase and uppercase"     '1\nmazes/small.txt\n4\nD\nd\nq\n5\n' "Moves: 2"
check "reaching the exit"           '1\nmazes/small.txt\n4\nd\nd\ns\ns\nd\nd\nd\n5\n' "You reached the exit in 7 moves" "# . # . . . P"
check "moving after finishing"      '1\nmazes/small.txt\n4\nd\nd\ns\ns\nd\nd\nd\n4\n5\n' "You already reached the exit"
check "exit ends move mode"         '1\nmazes/small.txt\n4\nd\nd\ns\ns\nd\nd\nd\n5\n' "Goodbye!"

# --- invalid input ---
check "invalid move commands"       '1\nmazes/small.txt\n4\nx\n\nwasd\n12\n  \nq\n5\n' "Invalid command. Use W, A, S, D"
check "invalid menu choices"        'abc\n9\n0\n\n12\n5\n'  "Invalid choice. Enter a number from 1 to 5." "Goodbye!"
check "very long menu line"         "$(printf 'z%.0s' $(seq 1 1000))\n5\n" "Invalid choice" "Goodbye!"
check "very long move line"         "1\nmazes/small.txt\n4\n$(printf 'w%.0s' $(seq 1 1000))\nq\n5\n" "Invalid command"
check "EOF at menu"                 ''  "Choice:"
check "EOF in move mode"            '1\nmazes/small.txt\n4\n'  "Move>"
check "EOF at path prompt"          '1\n'  "Maze file path"

# --- restart / reload ---
check "restart resets player"       '1\nmazes/small.txt\n4\nd\nd\nq\n3\n5\n'  "Player moved back to start." "Moves: 0"
check "restart after finishing"     '1\nmazes/small.txt\n4\nd\nd\ns\ns\nd\nd\nd\n3\n4\nd\nq\n5\n' "Player moved back to start." "Moves: 1"
check "load another maze"           '1\nmazes/small.txt\n4\nd\nq\n1\nmazes/sample.txt\n5\n' "(11 rows x 21 cols)" "Moves: 0"
check "bad load keeps old maze"     "1\nmazes/small.txt\n1\n$FIX/no_exit.txt\n2\n5\n" "Could not load maze" "# P . . # . #"

echo
echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
