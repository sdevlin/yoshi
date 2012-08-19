CC = gcc
FLAGS = -g -o $@
CFLAGS = -c -Wall -Werror -pedantic -std=c99 $(FLAGS)
LDFLAGS = $(FLAGS)

all : bin/yoshi

install : bin/yoshi
	cp bin/yoshi $(HOME)/bin/

bin/yoshi : main.o exp.o err.o gc.o read.o strbuf.o
	$(CC) $(LDFLAGS) $^

main.o : main.c data.h exp.h err.h gc.h read.h strbuf.h
	$(CC) $(CFLAGS) main.c

gc.o : gc.c gc.h data.h
	$(CC) $(CFLAGS) gc.c

read.o : read.c read.h strbuf.h
	$(CC) $(CFLAGS) read.c

exp.o : exp.c data.h exp.h err.h gc.h strbuf.h
	$(CC) $(CFLAGS) exp.c

err.o : err.c err.h
	$(CC) $(CFLAGS) err.c

strbuf.o : strbuf.c strbuf.h
	$(CC) $(CFLAGS) strbuf.c

clobber : clean
	rm -f bin/yoshi || true

clean :
	rm -f *.o || true

.PHONY : all install clobber clean
