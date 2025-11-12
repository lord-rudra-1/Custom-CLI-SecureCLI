CC = gcc
CFLAGS = -Wall -Wextra -fPIC
LDFLAGS = -lcrypto -lreadline -lncurses -ldl
TARGET = project
SOURCES = main.c commands.c file_management.c process_management.c terminal.c logger.c auth.c config.c dashboard.c plugin.c script.c
OBJECTS = $(SOURCES:.c=.o)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	@./launch.sh

plugin-example: plugins/example_plugin.c
	@mkdir -p plugins
	gcc -shared -fPIC -o plugins/example_plugin.so plugins/example_plugin.c

test-script: tests/test_script.c tests/test_harness.c script.c
	@mkdir -p tests
	gcc -o tests/test_script tests/test_script.c tests/test_harness.c script.c -I. -Wall
	@echo "Run with: ./tests/test_script"

test-plugin: tests/test_plugin.c tests/test_harness.c plugin.c
	@mkdir -p tests
	gcc -o tests/test_plugin tests/test_plugin.c tests/test_harness.c plugin.c -I. -ldl -Wall
	@echo "Run with: ./tests/test_plugin"

docs:
	doxygen Doxyfile
	@echo "Documentation generated in docs/html/"

.PHONY: clean run plugin-example test-script test-plugin docs

