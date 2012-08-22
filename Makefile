CC = gcc
FLAGS = -o $@
CFLAGS = -c -Wall -Werror -pedantic -std=c99 $(FLAGS)
LDFLAGS = $(FLAGS)

GC = gc_ms

debug : FLAGS += -g
debug : bin/yoshi

release : FLAGS += -O3
release : bin/yoshi

install : release
	cp bin/yoshi $(HOME)/bin/

prof : FLAGS += -pg
prof : debug

bin/yoshi : main.o exp.o env.o err.o $(GC).o read.o interp.o builtin.o print.o strbuf.o
	$(CC) $(LDFLAGS) $^

main.o : main.c flag.h exp.h env.h err.h gc.h read.h interp.h print.h builtin.h strbuf.h
	$(CC) $(CFLAGS) main.c

builtin.o : builtin.c builtin.h exp.h env.h err.h gc.h
	$(CC) $(CFLAGS) builtin.c

$(GC).o : $(GC).c gc.h flag.h
	$(CC) $(CFLAGS) $(GC).c

print.o : print.c print.h exp.h
	$(CC) $(CFLAGS) print.c

interp.o : interp.c interp.h exp.h env.h err.h gc.h
	$(CC) $(CFLAGS) interp.c

read.o : read.c read.h strbuf.h exp.h err.h
	$(CC) $(CFLAGS) read.c

env.o : env.c env.h err.h
	$(CC) $(CFLAGS) env.c

exp.o : exp.c exp.h err.h gc.h strbuf.h
	$(CC) $(CFLAGS) exp.c

err.o : err.c err.h
	$(CC) $(CFLAGS) err.c

strbuf.o : strbuf.c strbuf.h
	$(CC) $(CFLAGS) strbuf.c

clobber : clean
	rm -f bin/yoshi || true

clean :
	rm -f *.o || true

.PHONY : debug release install prof clobber clean
