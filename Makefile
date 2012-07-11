CC = gcc
FLAGS = -g -o $@
CFLAGS = -c -Wall -Werror -pedantic -std=c99 $(FLAGS)
LDFLAGS = $(FLAGS)

all : lisp

lisp : main.o
	$(CC) $(LDFLAGS) main.o

main.o : main.c
	$(CC) $(CFLAGS) main.c

clobber : clean
	rm -f lisp || true

clean :
	rm -f *.o || true

.PHONY : all clobber clean
