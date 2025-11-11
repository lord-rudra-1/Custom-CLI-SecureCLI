#ifndef AUTH_H
#define AUTH_H

#include <stdbool.h>

typedef struct {
    char username[50];
    char password_hash[65];
    char role[10];  // "admin" or "user"
} User;

extern User *current_user;

void load_users();
int login();
bool is_admin();

#endif