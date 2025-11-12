#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include "plugin.h"

#define MAX_PLUGINS 50
#define PLUGIN_DIR "./plugins"

static Plugin plugins[MAX_PLUGINS];
static int plugin_count = 0;

// Plugin initialization function signature
typedef void (*plugin_init_func)(void);

void plugin_init(void) {
    plugin_count = 0;
    memset(plugins, 0, sizeof(plugins));
}

int plugin_load(const char *path) {
    if (plugin_count >= MAX_PLUGINS) {
        fprintf(stderr, "Maximum number of plugins reached\n");
        return -1;
    }

    // Open the shared library
    void *handle = dlopen(path, RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Cannot load plugin %s: %s\n", path, dlerror());
        return -1;
    }

    // Look for the command function
    // Convention: plugin name is the filename without .so
    // Function name is plugin_<name>_command
    char func_name[256];
    const char *basename = strrchr(path, '/');
    if (!basename) basename = path;
    else basename++;

    // Extract plugin name (remove .so extension)
    // Limit to 80 chars to ensure snprintf doesn't truncate (80 + "plugin_" + "_command" = ~95 < 256)
    char plugin_name[80];
    size_t basename_len = strlen(basename);
    if (basename_len >= sizeof(plugin_name)) {
        basename_len = sizeof(plugin_name) - 1;
    }
    strncpy(plugin_name, basename, basename_len);
    plugin_name[basename_len] = '\0';
    char *ext = strstr(plugin_name, ".so");
    if (ext) *ext = '\0';
    // Ensure plugin_name is still within safe limits after .so removal
    size_t plugin_name_len = strlen(plugin_name);
    if (plugin_name_len >= sizeof(plugin_name)) {
        plugin_name[sizeof(plugin_name) - 1] = '\0';
        plugin_name_len = sizeof(plugin_name) - 1;
    }

    // Try different function name conventions
    // Check length to avoid truncation warning: "plugin_" (7) + name + "_command" (8) = 15 + name_len
    if (plugin_name_len + 15 < sizeof(func_name)) {
        snprintf(func_name, sizeof(func_name), "plugin_%s_command", plugin_name);
    } else {
        snprintf(func_name, sizeof(func_name), "%s_command", plugin_name);
    }
    plugin_command_func func = (plugin_command_func)dlsym(handle, func_name);

    if (!func) {
        // Try alternative: just the plugin name
        // Check length: name + "_command" (8) = 8 + name_len
        if (plugin_name_len + 8 < sizeof(func_name)) {
            snprintf(func_name, sizeof(func_name), "%s_command", plugin_name);
        } else {
            // Fallback: use first 240 chars
            strncpy(func_name, plugin_name, 240);
            strcat(func_name, "_command");
        }
        func = (plugin_command_func)dlsym(handle, func_name);
    }

    if (!func) {
        // Try: command (generic)
        func = (plugin_command_func)dlsym(handle, "command");
    }

    if (!func) {
        fprintf(stderr, "Plugin %s does not export a command function\n", path);
        dlclose(handle);
        return -1;
    }

    // Look for description function
    char desc_func_name[256];
    // Check length: "plugin_" (7) + name + "_description" (13) = 20 + name_len
    if (plugin_name_len + 20 < sizeof(desc_func_name)) {
        snprintf(desc_func_name, sizeof(desc_func_name), "plugin_%s_description", plugin_name);
    } else {
        strncpy(desc_func_name, plugin_name, 236);
        strcat(desc_func_name, "_description");
    }
    char *(*desc_func)(void) = (char *(*)(void))dlsym(handle, desc_func_name);
    
    char *description = NULL;
    if (desc_func) {
        description = desc_func();
    }

    // Call initialization function if it exists
    // Check length: "plugin_" (7) + name + "_init" (5) = 12 + name_len
    if (plugin_name_len + 12 < sizeof(func_name)) {
        snprintf(func_name, sizeof(func_name), "plugin_%s_init", plugin_name);
    } else {
        strncpy(func_name, plugin_name, 244);
        strcat(func_name, "_init");
    }
    plugin_init_func init_func = (plugin_init_func)dlsym(handle, func_name);
    if (init_func) {
        init_func();
    }

    // Register the plugin
    plugins[plugin_count].name = strdup(plugin_name);
    plugins[plugin_count].handle = handle;
    plugins[plugin_count].func = func;
    plugins[plugin_count].description = description ? strdup(description) : strdup("No description");

    printf("Loaded plugin: %s\n", plugin_name);
    plugin_count++;

    return 0;
}

void plugin_load_all(void) {
    // Check if plugins directory exists
    DIR *dir = opendir(PLUGIN_DIR);
    if (!dir) {
        // Create plugins directory if it doesn't exist
        mkdir(PLUGIN_DIR, 0755);
        dir = opendir(PLUGIN_DIR);
        if (!dir) {
            return; // Can't create or open directory
        }
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG || entry->d_type == DT_LNK) {
            const char *name = entry->d_name;
            if (strstr(name, ".so") != NULL) {
                char path[512];
                snprintf(path, sizeof(path), "%s/%s", PLUGIN_DIR, name);
                plugin_load(path);
            }
        }
    }

    closedir(dir);
}

void plugin_unload(const char *name) {
    for (int i = 0; i < plugin_count; i++) {
        if (strcmp(plugins[i].name, name) == 0) {
            if (plugins[i].handle) {
                dlclose(plugins[i].handle);
            }
            if (plugins[i].name) {
                free(plugins[i].name);
            }
            if (plugins[i].description) {
                free(plugins[i].description);
            }

            // Shift remaining plugins
            for (int j = i; j < plugin_count - 1; j++) {
                plugins[j] = plugins[j + 1];
            }
            plugin_count--;
            printf("Unloaded plugin: %s\n", name);
            return;
        }
    }
    fprintf(stderr, "Plugin not found: %s\n", name);
}

void plugin_cleanup(void) {
    for (int i = 0; i < plugin_count; i++) {
        if (plugins[i].handle) {
            dlclose(plugins[i].handle);
        }
        if (plugins[i].name) {
            free(plugins[i].name);
        }
        if (plugins[i].description) {
            free(plugins[i].description);
        }
    }
    plugin_count = 0;
}

Plugin *plugin_get(const char *name) {
    for (int i = 0; i < plugin_count; i++) {
        if (strcmp(plugins[i].name, name) == 0) {
            return &plugins[i];
        }
    }
    return NULL;
}

Plugin *plugin_get_all(int *count) {
    *count = plugin_count;
    return plugins;
}

