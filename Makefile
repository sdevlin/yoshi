CC = gcc
FLAGS = -g -o $@
CFLAGS = -c -Wall -Werror -pedantic -std=c99 $(FLAGS)
LDFLAGS = $(FLAGS)

all : bin/yoshi

install : bin/yoshi
	cp bin/yoshi $(HOME)/bin/

bin/yoshi : main.o
	$(CC) $(LDFLAGS) main.o

main.o : main.c
	$(CC) $(CFLAGS) main.c

clobber : clean
	rm -f bin/yoshi || true

clean :
	rm -f *.o || true

.PHONY : all install clobber clean
