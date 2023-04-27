#include "account.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

const char *FILENAME = "account.txt";
const int MAX_CLIENTS = 2;

// process the message and split it into digits and letters
int process_message(const char *buff, char *digits, char *letters) {
    int i = 0;
    int j = 0;
    int k = 0;
    while (buff[i] != '\0') {
        if (buff[i] >= '0' && buff[i] <= '9') {
            digits[j] = buff[i];
            ++j;
        } else if ((buff[i] >= 'a' && buff[i] <= 'z') || (buff[i] >= 'A' && buff[i] <= 'Z')) {
            letters[k] = buff[i];
            ++k;
        } else {
            return 0;
        }
        ++i;
    }
    digits[j] = '\0';
    letters[k] = '\0';
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./server <port_number>\n");
        return 0;
    }
    char *buff = (char *) malloc(MAX_CHARS * sizeof(char));
    // check if the port number is valid
    errno = 0;
    uint16_t PORT = strtoul(argv[1], &buff, 10);

    if (errno != 0 || *buff != '\0') {
        printf("Error: %s\n", errno == EINVAL ? "invalid base" : "invalid input");
        return 0;
    }
    // initialize the account list

    Account account_list = read_account(FILENAME);
    Account logged_in_user = account_list;
    while (logged_in_user) {
        logged_in_user->client_idx = -1; // no account is logged in
        logged_in_user = logged_in_user->next;
    }
    // initialize the socket
    int server_sock;
    struct sockaddr_in server;
    struct sockaddr_in client_tmp; /* temporary variable of client address information */
    struct sockaddr_in clients[MAX_CLIENTS]; /* a list of clients */
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        // initialize with blank address
        bzero(&(clients[i]), sizeof(struct sockaddr_in));
    }
    socklen_t clients_size[MAX_CLIENTS]; /* a list of clients' size */
    int clients_status[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        clients_status[i] = USERNAME_REQUIRED;
    }
    ssize_t bytes_sent, bytes_received;
    int sin_size;

    server_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_sock == -1) {
        perror("\nError: ");
        return 0;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;
    bzero(&(server.sin_zero), 8);
    // bind the socket to the port
    if (bind(server_sock, (struct sockaddr *) &server, sizeof(struct sockaddr)) == -1) {
        perror("\nError: ");
        return 0;
    }

    char *username = (char *) malloc(MAX_CHARS * sizeof(char));
    char *password = (char *) malloc(MAX_CHARS * sizeof(char));
    char *letters = (char *) malloc(MAX_CHARS * sizeof(char));
    char *digits = (char *) malloc(MAX_CHARS * sizeof(char));


    while (true) {
        int message_status = 0;
        memset(buff, '\0', MAX_CHARS);
        sin_size = sizeof(struct sockaddr_in);
        // receive the message from the client
        bytes_received = recvfrom(server_sock, buff, MAX_CHARS - 1, 0, (struct sockaddr *) &client_tmp,
                                  (socklen_t *) &sin_size);
        if (bytes_received < 0) {
            perror("\nError: ");
            return 0;
        }
        int idx = 0;
        bool is_new_client = true;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].sin_addr.s_addr == client_tmp.sin_addr.s_addr &&
                clients[i].sin_port == client_tmp.sin_port) {
                is_new_client = false;
                idx = i; // also get the index of the client_tmp
                clients_size[i] = sin_size; // update the size of the client_tmp
                break;
            }
        }
        if (is_new_client) {
            // add the client_tmp to the list
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].sin_addr.s_addr == 0 && clients[i].sin_port == 0) {
                    clients[i] = client_tmp;
                    idx = i; // also get the index of the client_tmp
                    clients_size[i] = sin_size; // update the size of the client_tmp
                    break;
                }
            }
            printf("New client connected: [%s:%d]\n", inet_ntoa(client_tmp.sin_addr), ntohs(client_tmp.sin_port));
        }
        printf("Received from client [%s:%d] %s\n", inet_ntoa(client_tmp.sin_addr), ntohs(client_tmp.sin_port), buff);
        if (strcmp(buff, "") != 0) { // check if the message is not empty
            // make the response base on the current server status
            if (clients_status[idx] == USERNAME_REQUIRED) {
                memset(username, '\0', MAX_CHARS);
                strcpy(username, buff);
                clients_status[idx] = PASSWORD_REQUIRED;
                strcpy(buff, "Insert password: ");
            } else if (clients_status[idx] == PASSWORD_REQUIRED) {
                memset(password, '\0', MAX_CHARS);
                strcpy(password, buff);
                int result = process_login(account_list, username, password, &logged_in_user);
                if (result == VALID_CREDENTIALS) {
                    clients_status[idx] = VALID_CREDENTIALS;
                    logged_in_user->client_idx = idx;
                    strcpy(buff, "OK.");
                } else if (result == ACCOUNT_NOT_EXIST) {
                    clients_status[idx] = USERNAME_REQUIRED;
                    strcpy(buff, "Account does not exist!");
                } else if (result == WRONG_PASSWORD) {
                    clients_status[idx] = USERNAME_REQUIRED;
                    strcpy(buff, "Not OK.");
                } else if (result == ACCOUNT_BLOCKED) {
                    clients_status[idx] = USERNAME_REQUIRED;
                    strcpy(buff, "Account is blocked");
                } else if (result == ACCOUNT_NOT_ACTIVE) {
                    clients_status[idx] = USERNAME_REQUIRED;
                    strcpy(buff, "Account is not ready");
                } else if (result == ACCOUNT_ALREADY_SIGN_IN) {
                    clients_status[idx] = USERNAME_REQUIRED;
                    strcpy(buff, "Account is already signed in");
                }
            } else if (clients_status[idx] == VALID_CREDENTIALS) {
                Account a = search_by_client_idx(account_list, idx);
                if (strcmp(buff, "bye") == 0) {
                    clients_status[idx] = USERNAME_REQUIRED;
                    strcpy(buff, "Goodbye ");
                    strcat(buff, a->username);
                    save_to_file(account_list, FILENAME);
                    a->client_idx = -1;
                } else if (strcmp(buff, "changepassword") == 0) {
                    clients_status[idx] = ACCOUNT_CHANGE_PASSWORD;
                    strcpy(buff, "Insert new password: ");
                } else {
                    message_status = process_message(buff, digits, letters);
                    if (message_status == 0) {
                        strcpy(buff, "Error");
                    }
                }
            } else if (clients_status[idx] == ACCOUNT_CHANGE_PASSWORD) {
                Account a = search_by_client_idx(account_list, idx);
                strcpy(a->password, buff);
                clients_status[idx] = VALID_CREDENTIALS;
                strcpy(buff, "Password changed successfully.");
            }
        } else {
            clients_status[idx] = USERNAME_REQUIRED;
            save_to_file(account_list, FILENAME);
            Account a = search_by_client_idx(account_list, idx);
            if (a) {
                a->client_idx = -1;
            }
        }

        if (message_status == 0) {
            bytes_sent = sendto(server_sock, buff, strlen(buff), 0, (struct sockaddr *) &client_tmp, sin_size);
            if (bytes_sent < 0) {
                perror("\nError: ");
                return 0;
            }
        } else if (search_by_client_idx(account_list, MAX_CLIENTS - idx - 1)) {
            bytes_sent = sendto(server_sock, digits, strlen(digits), 0,
                                (struct sockaddr *) &clients[MAX_CLIENTS - idx - 1],
                                clients_size[MAX_CLIENTS - idx - 1]);
            if (bytes_sent < 0) {
                perror("\nError: ");
                return 0;
            }
            bytes_sent = sendto(server_sock, letters, strlen(letters), 0,
                                (struct sockaddr *) &clients[MAX_CLIENTS - idx - 1],
                                clients_size[MAX_CLIENTS - idx - 1]);
            if (bytes_sent < 0) {
                perror("\nError: ");
                return 0;
            }
        }
    }
}