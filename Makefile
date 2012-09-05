CC = gcc
CFLAGS = -Wall -Werror -pedantic -std=c99

TARGET = bin/yoshi

GC_IMP = src/gc_ms.c

SOURCES = $(filter-out src/gc_%.c, $(wildcard src/**/*.c src/*.c))
SOURCES += $(GC_IMP)
OBJECTS = $(SOURCES:.c=.o)
DEPENDS = $(SOURCES:.c=.d)

dev: CFLAGS += -g
dev: $(TARGET)

prof: CFLAGS += -pg
prof: dev

release: CFLAGS += -O3
release: $(TARGET)

install: release
	install $(TARGET) $(HOME)/bin/

$(TARGET): $(OBJECTS)
	@mkdir -p bin
	$(CC) $(CFLAGS) $^ -o $(TARGET)

-include $(DEPENDS)

%.o: %.c
	$(CC) $(CFLAGS) -c $*.c -o $*.o
	@$(CC) $(CFLAGS) -MM $*.c >$*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' <$*.d.tmp >$*.d
	@sed -e 's/.*://' -e 's/\\$$//' <$*.d.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >>$*.d
	@rm -f $*.d.tmp

clobber: clean
	rm -f $(TARGET) || true

clean:
	rm -f $(OBJECTS) $(DEPENDS) || true

.PHONY: dev prof release install clobber clean
