#include "account.h"

// create new node of account
Account new_account(char *username, char *password, int attempts, int is_active) {
    Account acc = (Account) malloc(sizeof(struct account));
    acc->username = (char *) malloc(sizeof(char) * MAX_CHARS);
    acc->password = (char *) malloc(sizeof(char) * MAX_CHARS);
    strcpy(acc->username, username);
    strcpy(acc->password, password);
    acc->attempts = attempts;
    acc->is_active = is_active;
    acc->next = NULL;
    return acc;
}

// add new node to the end of the list
Account add_account(Account account_list, char *username, char *password, int attempts, int is_active) {
    Account new_acc = new_account(username, password, attempts, is_active);
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

// read account information from file
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
        account_list = add_account(account_list, username, password, 0, is_active);
    }
    fclose(f);
    return account_list;
}

// display account information
void show_account(Account account_list) {
    printf("%-20s%-20s%-20s%-20s", "Username", "Password", "Attempts", "Is Active");
    printf("\n");
    Account tmp = account_list;
    while (tmp != NULL) {
        printf("%-20s%-20s%-20d%-20d", tmp->username, tmp->password, tmp->attempts, tmp->is_active);
        printf("\n");
        tmp = tmp->next;
    }
}

// process login and return corresponding status
// also keep track of number of attempts
// and save successfully logged-in user to current_account
int process_login(Account account_list, char *username, char *password, Account *current_account) {
    // should return 2 to 5, base on defined constants in account.h
    Account tmp = account_list;
    while (tmp != NULL) {
        if (strcmp(tmp->username, username) == 0) { // found username
            if (strcmp(tmp->password, password) == 0) { // found password
                if (tmp->is_active == 1) { // account is active
                    *current_account = tmp;
                    return VALID_CREDENTIALS;
                } else if (tmp->attempts >= MAX_ATTEMPTS) { // account is inactive
                    return ACCOUNT_BLOCKED;
                } else { // account is inactive
                    return ACCOUNT_NOT_ACTIVE;
                }
            } else { // wrong password
                tmp->attempts++;
                if (tmp->attempts >= MAX_ATTEMPTS) { // account is blocked
                    tmp->is_active = 0;
                    return ACCOUNT_BLOCKED;
                } else { // account is still active
                    return WRONG_PASSWORD;
                }
            }
        }
        tmp = tmp->next;
    }
    return ACCOUNT_NOT_EXIST;
}

// save account information to file
void save_to_file(Account account_list, const char *filename) {
    FILE *f = fopen(filename, "w+");
    if (f == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    Account tmp = account_list;
    while (tmp->next != NULL) {
        fprintf(f, "%s %s %d\n", tmp->username, tmp->password, tmp->is_active);
        tmp = tmp->next;
    }
    fprintf(f, "%s %s %d", tmp->username, tmp->password, tmp->is_active);
    fclose(f);
}
