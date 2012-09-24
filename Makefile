CC = gcc
CFLAGS = -Wall -Werror -pedantic -std=c99 -D PREFIX=\"$(PREFIX)\"

TARGET = bin/yoshi
STDLIB = lib/yoshi/stdlib.scm

PREFIX ?= $(CURDIR)

GC ?= src/gc_ms.c

SRCS = $(filter-out src/gc_%.c, $(wildcard src/**/*.c src/*.c))
SRCS += $(GC)
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

dev: CFLAGS += -g
dev: $(TARGET)

prof: CFLAGS += -pg
prof: dev

release: CFLAGS += -O3
release: $(TARGET)

install: PREFIX = $(HOME)
install: release
	install -D $(TARGET) $(PREFIX)/$(TARGET)
	install -m 644 -D $(STDLIB) $(PREFIX)/$(STDLIB)

$(TARGET): $(OBJS)
	@mkdir -p bin
	$(CC) $(CFLAGS) $^ -o $(TARGET)

-include $(DEPS)

%.o: %.c
	$(CC) $(CFLAGS) -c $*.c -o $*.o
	@$(CC) $(CFLAGS) -MM $*.c >$*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' <$*.d.tmp >$*.d
	@sed -e 's/.*://' -e 's/\\$$//' <$*.d.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >>$*.d
	@rm -f $*.d.tmp

clobber: clean
	rm -r bin || true

clean:
	rm -f $(OBJS) $(DEPS) || true

.PHONY: dev prof release install clobber clean
