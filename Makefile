CC ?= cc
CFLAGS ?= -std=c17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow
LDLIBS =

TARGET = newshell
SOURCES = main.c commands.c pipeline.c redirection.c signalControl.c
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all clean test sanitize

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDLIBS)

%.o: %.c main.h
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TARGET)
	./tests/test_newshell.sh ./$(TARGET)

sanitize:
	$(CC) $(CFLAGS) -g -fsanitize=address,undefined -fno-omit-frame-pointer \
		$(SOURCES) -o $(TARGET)-sanitize $(LDLIBS)
	./tests/test_newshell.sh ./$(TARGET)-sanitize

clean:
	rm -f $(TARGET) $(TARGET)-sanitize $(OBJECTS)
	rm -rf $(TARGET)-sanitize.dSYM
