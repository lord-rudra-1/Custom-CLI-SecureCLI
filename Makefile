CC = gcc
CFLAGS = -Wall -Wextra -fPIC
LDFLAGS = -lcrypto -lreadline -lncurses
TARGET = project
SOURCES = main.c commands.c file_management.c process_management.c terminal.c logger.c auth.c config.c crypto.c dashboard.c script.c
OBJECTS = $(SOURCES:.c=.o)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	@./launch.sh

test-script: tests/test_script.c tests/test_harness.c script.c
	@mkdir -p tests
	gcc -o tests/test_script tests/test_script.c tests/test_harness.c script.c -I. -Wall
	@echo "Run with: ./tests/test_script"

docs:
	doxygen Doxyfile
	@echo "Documentation generated in docs/html/"

.PHONY: clean run test-script docs

