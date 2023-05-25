#include "account.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Account new_account(char *username, char *password, int attempts, int is_active, int is_signed_in) {
    Account acc = (Account) malloc(sizeof(struct account));
    acc->username = (char *) malloc(sizeof(char) * MAX_CHARS);
    acc->password = (char *) malloc(sizeof(char) * MAX_CHARS);
    strcpy(acc->username, username);
    strcpy(acc->password, password);
    acc->is_active = is_active;
    acc->attempts = attempts;
    acc->is_signed_in = is_signed_in;
    acc->next = NULL;
    return acc;
}

Account add_account(Account account_list, char *username, char *password, int attempts, int is_active, int is_signed_in) {
    Account new_acc = new_account(username, password, attempts, is_active, is_signed_in);
    if (account_list == NULL) {
        account_list = new_acc;
    } else {
        Account tmp = account_list;
        while (tmp->next != NULL) {
            tmp = tmp->next;
        }
        tmp->next = new_acc;
    }
    return account_list;
}

Account read_account(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        printf("Cannot open file %s to read account information !\n", filename);
        return NULL;
    }
    Account account_list = NULL;
    char *username = (char *) malloc(sizeof(char) * MAX_CHARS);
    char *password = (char *) malloc(sizeof(char) * MAX_CHARS);
    int is_active;
    while (fscanf(f, "%s %s %d", username, password, &is_active) != EOF) {
        account_list = add_account(account_list, username, password, 0, is_active, 0);
    }
    fclose(f);
    return account_list;
}

int process_login(Account account_list, char *username, char *password) {
    Account tmp = account_list;
    while (tmp != NULL) {
        if (strcmp(tmp->username, username) == 0) {
            if (strcmp(tmp->password, password) == 0) {
                if (tmp->is_signed_in) {
                    return ACCOUNT_ALREADY_SIGNED_IN; // account already signed in
                }
                else if (tmp->is_active) {
                    tmp->is_signed_in = 1;
                    tmp->attempts = 0;
                    return VALID_CREDENTIALS; // valid credentials
                } else {
                    return ACCOUNT_NOT_ACTIVE; // credentials correct but account is not active
                }
            } else {
                tmp->attempts++;
                if (tmp->attempts >= MAX_ATTEMPTS) {
                    tmp->is_active = 0;
                    return ACCOUNT_BLOCKED; // account blocked due to too many failed attempts
                } else {
                    return WRONG_PASSWORD; // username found, but wrong password
                }
            }
        }
        tmp = tmp->next;
    }
    return USERNAME_REQUIRED; // username not found
}

void save_to_file(Account account_list, const char *filename) {
    FILE *f = fopen(filename, "w+");
    if (f == NULL) {
        printf("Cannot open file %s to save account information !\n", filename);
        return;
    }
    Account tmp = account_list;
    while (tmp->next != NULL) {
        fprintf(f, "%s %s %d\n", tmp->username, tmp->password, tmp->is_active);
        tmp = tmp->next;
    }
    fprintf(f, "%s %s %d", tmp->username, tmp->password, tmp->is_active);
    fclose(f);
}