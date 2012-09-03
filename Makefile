CC = gcc
CFLAGS = -Wall -Werror -pedantic -std=c99

TARGET = bin/yoshi

GC_IMP = src/gc_ms.c

SOURCES = $(filter-out src/gc_%.c, $(wildcard src/**/*.c src/*.c))
SOURCES += $(GC_IMP)
OBJECTS = $(patsubst %.c,%.o,$(SOURCES))

dev : CFLAGS += -g
dev : $(TARGET)

prof : CFLAGS += -pg
prof : dev

release : CFLAGS += -O3
release : $(TARGET)

install : release
	install $(TARGET) $(HOME)/bin/

$(TARGET) : $(OBJECTS)
	@mkdir -p bin
	$(CC) $(LDFLAGS) -o $(TARGET) $^

clobber : clean
	rm -f $(TARGET) || true

clean :
	rm -f $(OBJECTS) || true

.PHONY : debug prof release install clobber clean
