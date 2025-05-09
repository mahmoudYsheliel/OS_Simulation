CC=gcc
# GLIB_C_FLAGS=$(shell pkg-config --cflags --libs glib-2.0)
FLAGS=-lpthread

all: process_generator scheduler process utils

process_generator: process_generator.c
	$(CC) process_generator.c $(FLAGS) -o ./bin/process_generator -g

scheduler: scheduler.c
	$(CC) scheduler.c $(FLAGS) -lm -o ./bin/scheduler -g

process: process.c
	$(CC) process.c $(FLAGS) -o ./bin/process -g

utils: test_generator.c clk.c
	$(CC) test_generator.c $(FLAGS) -o ./bin/test_generator -g
	$(CC) clk.c $(FLAGS) -o ./bin/clk -g
	cp ./processes_data.txt ./bin/

clean:
	rm ./bin/*