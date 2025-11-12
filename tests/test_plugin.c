#include "test_harness.h"
#include "../plugin.h"

static void test_plugin_init(void) {
    plugin_init();
    int count;
    Plugin *plugins = plugin_get_all(&count);
    TEST_ASSERT_EQ(0, count, "Plugin count should be 0 after init");
    plugin_cleanup();
}

static void test_plugin_get_nonexistent(void) {
    plugin_init();
    Plugin *plugin = plugin_get("nonexistent");
    TEST_ASSERT(plugin == NULL, "Should return NULL for nonexistent plugin");
    plugin_cleanup();
}

int main(void) {
    printf("=== Plugin Module Tests ===\n\n");
    
    RUN_TEST(test_plugin_init);
    RUN_TEST(test_plugin_get_nonexistent);
    
    printf("\n=== Test Summary ===\n");
    printf("Total: %d, Passed: %d, Failed: %d\n", tests_total, tests_passed, tests_failed);
    
    return (tests_failed == 0) ? 0 : 1;
}

