CC      = gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -O2 -g
TARGET  := maze_solver
SRCS    := $(wildcard src/*.c)
OBJS    := $(SRCS:src/%.c=build/%.o)
LIB_OBJS := $(filter-out build/main.o,$(OBJS))

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

build/%.o: src/%.c $(wildcard src/*.h) | build
	$(CC) $(CFLAGS) -c -o $@ $<

build:
	mkdir -p build

# Solver unit tests. --wrap=malloc lets the tests simulate allocation failure.
build/test_solver: tests/test_solver.c $(LIB_OBJS) | build
	$(CC) $(CFLAGS) -Isrc -Wl,--wrap=malloc -o $@ $^

test: $(TARGET) build/test_solver
	./build/test_solver
	bash tests/run_tests.sh

clean:
	rm -rf build $(TARGET)

.PHONY: test clean
