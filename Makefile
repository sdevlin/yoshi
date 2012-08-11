CC = gcc
FLAGS = -g -o $@
CFLAGS = -c -Wall -Werror -pedantic -std=c99 $(FLAGS)
LDFLAGS = $(FLAGS)

all : yoshi

install : yoshi
	cp yoshi ~/bin/

yoshi : main.o
	$(CC) $(LDFLAGS) main.o

main.o : main.c
	$(CC) $(CFLAGS) main.c

clobber : clean
	rm -f yoshi || true

clean :
	rm -f *.o || true

.PHONY : all clobber clean
