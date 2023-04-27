#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CHARS 1000

#define MAX_ATTEMPTS 3

// 0: username required - both username and password are now not provided
// 0 can also be used for signed out status
// 1: password required - username is provided but password is not provided
// 2: valid credentials - both username and password are provided and they are valid
// 3: account does not exist - username is provided but it does not exist
// 4: wrong password - username is provided but password is invalid
// 5: account blocked - username is provided but the account is blocked/deactivated

#define USERNAME_REQUIRED 0
#define PASSWORD_REQUIRED 1
#define VALID_CREDENTIALS 2
#define ACCOUNT_NOT_EXIST 3
#define WRONG_PASSWORD 4
#define ACCOUNT_BLOCKED 5
#define ACCOUNT_NOT_ACTIVE 6
#define ACCOUNT_ALREADY_SIGN_IN 7
#define ACCOUNT_CHANGE_PASSWORD 8


struct account {
    char *username;
    char *password;
    int attempts;
    int is_active;
    int client_idx;
    struct account *next;
};

typedef struct account *Account;

Account new_account(char *, char *, int, int);

Account add_account(Account, char *, char *, int, int);

Account read_account(const char *);

void show_account(Account);

int process_login(Account, char *, char *, Account *);

void save_to_file(Account, const char *);

Account search_by_client_idx(Account, int);
#endif