// Example plugin for SecureSysCLI
// Compile with: gcc -shared -fPIC -o example_plugin.so example_plugin.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Plugin command function - this is what gets called when user types "example"
void plugin_example_plugin_command(int argc, char *argv[]) {
    printf("Hello from example plugin!\n");
    if (argc > 1) {
        printf("Arguments received: ");
        for (int i = 1; i < argc; i++) {
            printf("%s ", argv[i]);
        }
        printf("\n");
    }
}

// Optional: Plugin description function
char *plugin_example_plugin_description(void) {
    return "Example plugin demonstrating plugin architecture";
}

// Optional: Plugin initialization function
void plugin_example_plugin_init(void) {
    printf("Example plugin initialized!\n");
}



