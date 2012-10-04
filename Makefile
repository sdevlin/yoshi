CC = gcc
CFLAGS = -Wall -Werror -pedantic -std=c99 -D PREFIX=\"$(PREFIX)\"

TARGET = bin/yoshi
STDLIB = lib/yoshi/stdlib.scm

PREFIX ?= $(CURDIR)

SRCS = $(wildcard src/**/*.c src/*.c)
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

dev: CFLAGS += -g
dev: $(TARGET)

prof: CFLAGS += -pg
prof: dev

release: PREFIX = $(HOME)
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

# the black magic after the first line constructs the dep files correctly
%.o: %.c
	$(CC) $(CFLAGS) -c $*.c -o $*.o
	@$(CC) $(CFLAGS) -MM $*.c >$*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' <$*.d.tmp >$*.d
	@sed -e 's/.*://' -e 's/\\$$//' <$*.d.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >>$*.d
	@rm -f $*.d.tmp

check-syntax:
	$(CC) -o /dev/null -S $(CHK_SOURCES)

clobber: clean
	rm -f $(TARGET) || true

clean:
	rm -f $(OBJS) $(DEPS) || true

.PHONY: dev prof release install check-syntax clobber clean
