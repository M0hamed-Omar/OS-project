CC = gcc
CFLAGS = -Wall -g

DS_SRC   = $(wildcard data_structures/*.c)
DS_OBJ   = $(DS_SRC:.c=.o)

HDR_OBJ  = headers.o
MMU_OBJ  = MMU.o

ALL_OBJS = $(DS_OBJ) $(HDR_OBJ) $(MMU_OBJ)

TARGETS = process_generator.out clk.out scheduler.out process.out test_generator.out

.PHONY: all build clean run

all: build

# ---------------- build everything ----------------
build: $(ALL_OBJS) $(TARGETS)

# ---------------- compile folders ----------------
data_structures/%.o: data_structures/%.c headers.h
	$(CC) $(CFLAGS) -c $< -o $@

# compile headers.c
headers.o: headers.c headers.h
	$(CC) $(CFLAGS) -c $< -o $@

# compile MMU.c
MMU.o: MMU.c headers.h
	$(CC) $(CFLAGS) -c $< -o $@

# ---------------- main executables ----------------

process_generator.out: process_generator.c $(ALL_OBJS) headers.h
	$(CC) $(CFLAGS) $^ -o $@

clk.out: clk.c $(ALL_OBJS) headers.h
	$(CC) $(CFLAGS) $^ -o $@

scheduler.out: scheduler.c $(ALL_OBJS) headers.h
	$(CC) $(CFLAGS) $^ -o $@ -lm

process.out: process.c $(ALL_OBJS) headers.h
	$(CC) $(CFLAGS) $^ -o $@

test_generator.out: test_generator.c $(ALL_OBJS) headers.h
	$(CC) $(CFLAGS) $^ -o $@

# ---------------- clean ----------------
clean:
	rm -f $(ALL_OBJS) *.o *.out
	rm -f scheduler.log scheduler.perf
	rm -f scheduler_1.log scheduler_1.perf
	rm -f scheduler_2.log scheduler_2.perf
	rm -f memory.log
	rm -f output.txt

# ---------------- run ----------------
run: build
	./process_generator.out