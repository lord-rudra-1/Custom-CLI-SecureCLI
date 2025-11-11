CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lcrypto -lreadline
TARGET = project
SOURCES = main.c commands.c file_management.c process_management.c terminal.c logger.c auth.c config.c
OBJECTS = $(SOURCES:.c=.o)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	@./launch.sh

.PHONY: clean run

