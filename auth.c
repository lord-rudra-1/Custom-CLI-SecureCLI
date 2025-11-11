#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>
#include "auth.h"
#include <stdbool.h>
#include <termios.h>
#include <unistd.h>
#define MAX_USERS 10
#define USER_FILE "users.db"

User users[MAX_USERS];
int user_count = 0;
User *current_user = NULL;

// Convert hash bytes to hex string
static void to_hex(unsigned char *hash, char *output) {
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        sprintf(output + (i * 2), "%02x", hash[i]);
}

// Hash a password using SHA256
static void hash_password(const char *password, char *output) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char *)password, strlen(password), hash);
    to_hex(hash, output);
}

// Load users from users.db
void load_users() {
    FILE *f = fopen(USER_FILE, "r");
    if (!f) {
        printf("⚠️  No user database found (%s).\n", USER_FILE);
        return;
    }
    while (fscanf(f, "%49s %64s %9s",
                  users[user_count].username,
                  users[user_count].password_hash,
                  users[user_count].role) == 3) {
        user_count++;
        if (user_count >= MAX_USERS) break;
    }
    fclose(f);
}

// Helper function to read password without echoing
static void read_password(char *password, size_t max_len) {
    struct termios old_term, new_term;
    
    // Get current terminal settings
    tcgetattr(STDIN_FILENO, &old_term);
    new_term = old_term;
    
    // Disable echo
    new_term.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);
    
    // Read password
    if (fgets(password, max_len, stdin) == NULL) {
        password[0] = '\0';
    } else {
        // Remove newline if present
        size_t len = strlen(password);
        if (len > 0 && password[len - 1] == '\n') {
            password[len - 1] = '\0';
        }
    }
    
    // Restore terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
    printf("\n");  // Print newline after password input
}

// Login function
int login() {
    char user[50], pass[50], hash[65];
    int c;

    printf("Username: ");
    if (scanf("%49s", user) != 1) {
        return 0;
    }
    
    // Clear input buffer
    while ((c = getchar()) != '\n' && c != EOF);

    printf("Password: ");
    fflush(stdout);  // Ensure prompt is displayed
    read_password(pass, sizeof(pass));

    hash_password(pass, hash);

    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, user) == 0 &&
            strcmp(users[i].password_hash, hash) == 0) {
            current_user = &users[i];
            printf("✅  Login successful. Welcome %s (%s)\n",
                   user, current_user->role);
            return 1;
        }
    }
    printf("❌  Invalid credentials.\n");
    return 0;
}

bool is_admin() {
    return current_user && strcmp(current_user->role, "admin") == 0;
}
