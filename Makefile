CC = gcc
CFLAGS = -Wall -Wextra
TARGET = project
SOURCES = main.c commands.c file_management.c process_management.c terminal.c
OBJECTS = $(SOURCES:.c=.o)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	@./launch.sh

.PHONY: clean run

