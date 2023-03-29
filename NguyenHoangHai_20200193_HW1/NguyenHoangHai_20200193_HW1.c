#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MAX 1000

const char *FILE_NAME = "account.txt"; // file name to store account information

char *current_user = NULL; // current user that is logged in

struct account { // account structure
    char *username;
    char *password;
    bool is_active;
    struct account *next;
};

typedef struct account *Account;

Account list_account = NULL; // list of accounts (head of linked list)

Account new_account(char *username, char *password, bool is_active) { // create a new account (node of linked list)
    Account acc = (Account) malloc(sizeof(struct account));
    acc->username = (char *) malloc(sizeof(char) * MAX);
    acc->password = (char *) malloc(sizeof(char) * MAX);
    strcpy(acc->username, username);
    strcpy(acc->password, password);
    acc->is_active = is_active;
    acc->next = NULL;
    return acc;
}

void add_account(Account acc) { // add a new account to the list of accounts (linked list)
    if (list_account == NULL) {
        list_account = acc;
    } else {
        Account tmp = list_account;
        while (tmp->next != NULL) {
            tmp = tmp->next;
        }
        tmp->next = acc;
    }
}

void read_account_info(const char *filename) { // read account information from file
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    char *username = (char *) malloc(sizeof(char) * MAX);
    char *password = (char *) malloc(sizeof(char) * MAX);
    int is_active;
    while (fscanf(f, "%s %s %d", username, password, &is_active) != EOF) {
        add_account(new_account(username, password, is_active ? true : false));
    }
    fclose(f);
}

void write_account_info(const char *filename) { // write account information to file
    FILE *f = fopen(filename, "w+");
    if (f == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    Account tmp = list_account;
    while (tmp->next != NULL) {
        fprintf(f, "%s %s %d\n", tmp->username, tmp->password, tmp->is_active);
        tmp = tmp->next;
    }
    fprintf(f, "%s %s %d", tmp->username, tmp->password, tmp->is_active);
    fclose(f);
}

void menu() {
    printf("USER MANAGEMENT PROGRAM\n");
    printf("-----------------------------------\n");
    printf("1. Register\n");
    printf("2. Sign in\n");
    printf("3. Search\n");
    printf("4. Sign out\n");
    printf("Your choice (1-4, other to quit): ");
}

void display_account_list() {
    Account tmp = list_account;
    while (tmp != NULL) {
        printf("%s %s %s\n", tmp->username, tmp->password, tmp->is_active ? "active" : "blocked");
        tmp = tmp->next;
    }
}

Account search_account(char *username) { // search an account by username
    Account tmp = list_account;
    while (tmp != NULL) { // traverse the linked list
        if (strcmp(tmp->username, username) == 0) {
            return tmp; // return the account if found
        }
        tmp = tmp->next;
    }
    return NULL;    // return NULL if not found
}


/* login attempt structure
 * only store user that used to fail to login
 * every time a user failed to login, the number of attempts is increased by 1
 * if the number of attempts is greater than 3, the account is blocked
 * if the user successfully login, the number of attempts is reset to 0
 */

struct login_attempt {
    char *username;
    int number_of_attempts;
    struct login_attempt *next;
};

struct login_attempt *list_login_attempt = NULL;

struct login_attempt *new_login_attempt(char *username) {
    struct login_attempt *attempt = (struct login_attempt *) malloc(sizeof(struct login_attempt));
    attempt->username = (char *) malloc(sizeof(char) * MAX);
    strcpy(attempt->username, username);
    attempt->number_of_attempts = 1; // the first time the user failed to login
    attempt->next = NULL;
    return attempt;
}

void add_login_attempt(struct login_attempt *attempt) {
    if (list_login_attempt == NULL) {
        list_login_attempt = attempt;
    } else {
        struct login_attempt *tmp = list_login_attempt;
        while (tmp->next != NULL) {
            tmp = tmp->next;
        }
        tmp->next = attempt;
    }
}

void increase_login_attempt(char *username) {
    struct login_attempt *tmp = list_login_attempt;
    while (tmp != NULL) {
        if (strcmp(tmp->username, username) == 0) { // if the user is already in the list
            tmp->number_of_attempts++; // increase the number of attempts
            return;
        }
        tmp = tmp->next;
    }
    add_login_attempt(new_login_attempt(
            username)); // if the user is not in the list, add it to the list with number of attempts = 1
}

void reset_login_attempt(char *username) {
    struct login_attempt *tmp = list_login_attempt;
    while (tmp != NULL) {
        if (strcmp(tmp->username, username) == 0) { // if the user is in the list
            tmp->number_of_attempts = 0; // reset the number of attempts
            return;
        }
        tmp = tmp->next;
    }
}

int get_number_of_attempts(char *username) {
    struct login_attempt *tmp = list_login_attempt;
    while (tmp != NULL) {
        if (strcmp(tmp->username, username) == 0) {
            return tmp->number_of_attempts;
        }
        tmp = tmp->next;
    }
    return 0;
}

void process_register() {
    char *username = (char *) malloc(sizeof(char) * MAX);
    printf("Username: ");
    scanf("%s", username);
    if (search_account(username) != NULL) { // check if the account already exists
        printf("Account existed\n");
    } else { // if the account does not exist, create a new account
        char *password = (char *) malloc(sizeof(char) * MAX);
        printf("Password: ");
        scanf("%s", password);
        add_account(new_account(username, password, true));
        printf("Successful registration\n");
        write_account_info(FILE_NAME); // write the updated list of accounts to file
        free(password);
    }
    free(username);
}

void process_sign_in() {
    if (current_user != NULL) { // check if there is already a user signed in
        printf("You have already signed in as %s\nSign out before sign in.\n", current_user);
        return;
    }
    char *username = (char *) malloc(sizeof(char) * MAX);
    printf("Username: ");
    scanf("%s", username);
    Account acc = search_account(username);
    if (acc == NULL) { // check if the account exists
        printf("Cannot find account\n");
    } else if (!acc->is_active) { // check if the account is blocked
        printf("Account is blocked\n");
    } else {
        char *password = (char *) malloc(sizeof(char) * MAX);
        printf("Password: ");
        scanf("%s", password);
        if (strcmp(acc->password, password) != 0) { // check if the password is correct
            increase_login_attempt(username);
            if (get_number_of_attempts(username) >= 3) {
                acc->is_active = false;
                write_account_info(FILE_NAME);
                printf("Password is incorrect. Account is blocked\n");
            } else {
                printf("Password is incorrect\n");
            }
        } else { // if the password is correct
            reset_login_attempt(username);
            current_user = acc->username;
            printf("Hello %s\n", current_user);
        }
        free(password);
    }
    free(username);
}

void process_search() {
    char *username = (char *) malloc(sizeof(char) * MAX);
    printf("Username: ");
    scanf("%s", username);
    Account acc = search_account(username);
    if (acc == NULL) { // check if the account exists
        printf("Cannot find account\n");
    } else {
        printf("Account is %s\n", acc->is_active ? "active" : "blocked");
    }
    free(username);
}

void process_sign_out() {
    if (current_user == NULL) { // no one has signed in
        printf("No user signed in\n");
        return;
    }
    char *username = (char *) malloc(sizeof(char) * MAX);
    printf("Username: ");
    scanf("%s", username);
    Account acc = search_account(username);
    if (acc == NULL) { // check if the account exists
        printf("Cannot find account\n");
    } else if (strcmp(current_user, username) != 0) { // the user is not the one who is signed in
        printf("Account is not sign in\n");
    } else { //
        current_user = NULL;
        printf("Goodbye %s\n", username);
    }
    free(username);
}

void free_list_account() {
    struct account *tmp = list_account;
    while (tmp != NULL) {
        struct account *next = tmp->next;
        free(tmp->username);
        free(tmp->password);
        free(tmp);
        tmp = next;
    }
}

void free_list_login_attempt() {
    struct login_attempt *tmp = list_login_attempt;
    while (tmp != NULL) {
        struct login_attempt *next = tmp->next;
        free(tmp->username);
        free(tmp);
        tmp = next;
    }
}

int main() {
    read_account_info(FILE_NAME); // load the list of accounts from file
    int choice = 0;
    while (true) {
        menu();
        scanf("%d", &choice);
        switch (choice) {
            case 1:
                process_register();
                break;
            case 2:
                process_sign_in();
                break;
            case 3:
                process_search();
                break;
            case 4:
                process_sign_out();
                break;
            default:
                free_list_account();
                free_list_login_attempt();
                printf("Bye\n");
                return 0;
        }
    }
}
