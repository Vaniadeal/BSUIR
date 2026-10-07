CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -g

all: tests experiments

tests: tests.c mmemory.c mmemory.h
	$(CC) $(CFLAGS) tests.c mmemory.c -o tests

experiments: experiments.c mmemory.c mmemory.h
	$(CC) $(CFLAGS) experiments.c mmemory.c -o experiments

run: tests
	./tests

exp: experiments
	./experiments

clean:
	rm -f tests experiments *.o *.csv *.png