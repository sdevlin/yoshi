CC = gcc
FLAGS = -o $@
CFLAGS = -c -Wall -Werror -pedantic -std=c99 $(FLAGS)
LDFLAGS = $(FLAGS)

GC = gc_ms

dev : FLAGS += -g
dev : bin/yoshi

prof : FLAGS += -pg
prof : debug

release : FLAGS += -O3
release : bin/yoshi

install : release
	install bin/yoshi $(HOME)/bin/

bin/yoshi : main.o exp.o env.o err.o $(GC).o read.o interp.o builtin.o print.o strbuf.o
	@mkdir -p bin
	$(CC) $(LDFLAGS) $^

main.o : main.c flag.h env.h err.h gc.h read.h interp.h print.h builtin.h
	$(CC) $(CFLAGS) main.c

builtin.o : builtin.c builtin.h exp.h env.h err.h gc.h
	$(CC) $(CFLAGS) builtin.c

gc_ms.o : gc_ms.c exp.h env.h flag.h
	$(CC) $(CFLAGS) gc_ms.c

gc_nop.o : gc_nop.c exp.h env.h
	$(CC) $(CFLAGS) gc_nop.c

print.o : print.c exp.h
	$(CC) $(CFLAGS) print.c

interp.o : interp.c interp.h exp.h env.h err.h gc_alloc.h
	$(CC) $(CFLAGS) interp.c

read.o : read.c strbuf.h exp.h err.h
	$(CC) $(CFLAGS) read.c

env.o : env.c env.h exp.h err.h
	$(CC) $(CFLAGS) env.c

exp.o : exp.c exp.h err.h gc_alloc.h strbuf.h
	$(CC) $(CFLAGS) exp.c

err.o : err.c err.h
	$(CC) $(CFLAGS) err.c

strbuf.o : strbuf.c
	$(CC) $(CFLAGS) strbuf.c

clobber : clean
	rm -f bin/yoshi || true

clean :
	rm -f *.o || true

.PHONY : debug prof release install clobber clean
