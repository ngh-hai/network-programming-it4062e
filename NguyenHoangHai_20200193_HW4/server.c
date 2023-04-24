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
const int BUFF_SIZE = 1024;

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
    char *buff = (char *) malloc(BUFF_SIZE * sizeof(char));
    // check if the port number is valid
    errno = 0;
    uint16_t PORT = strtoul(argv[1], &buff, 10);

    if (errno != 0 || *buff != '\0') {
        printf("Error: %s\n", errno == EINVAL ? "invalid base" : "invalid input");
        return 0;
    }

    int status = USERNAME_REQUIRED;
    Account account_list = read_account(FILENAME);
    Account logged_in_user = NULL;

    int server_sock;
    struct sockaddr_in server;
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

    if (bind(server_sock, (struct sockaddr *) &server, sizeof(struct sockaddr)) == -1) {
        perror("\nError: ");
        return 0;
    }

    char *username = (char *) malloc(BUFF_SIZE * sizeof(char));
    char *password = (char *) malloc(BUFF_SIZE * sizeof(char));
    char *letters = (char *) malloc(BUFF_SIZE * sizeof(char));
    char *digits = (char *) malloc(BUFF_SIZE * sizeof(char));


    while (true) {
        int message_status = 0;
        memset(buff, '\0', strlen(buff));
        sin_size = sizeof(struct sockaddr_in);


        bytes_received = recvfrom(server_sock, buff, BUFF_SIZE, 0, (struct sockaddr *) &server,
                                  (socklen_t * ) & sin_size);
        if (bytes_received < 0) {
            perror("\nError: ");
            return 0;
        }
        buff[bytes_received] = '\0';


        printf("Received from client[%s:%d] %s\n", inet_ntoa(server.sin_addr), ntohs(server.sin_port), buff);

        if (strcmp(buff, "") != 0) {
            if (status == USERNAME_REQUIRED) {
                memset(username, '\0', strlen(username));
                strcpy(username, buff);
                status = PASSWORD_REQUIRED;
                strcpy(buff, "Insert password: ");
            } else if (status == PASSWORD_REQUIRED) {
                memset(password, '\0', strlen(password));
                strcpy(password, buff);
                int result = process_login(account_list, username, password, &logged_in_user);
                if (result == VALID_CREDENTIALS) {
                    status = VALID_CREDENTIALS;
                    strcpy(buff, "OK. Welcome to the server!");
                } else if (result == ACCOUNT_NOT_EXIST) {
                    status = USERNAME_REQUIRED;
                    strcpy(buff, "Account does not exist!");
                } else if (result == WRONG_PASSWORD) {
                    status = USERNAME_REQUIRED;
                    strcpy(buff, "Not OK. Wrong password!");
                } else if (result == ACCOUNT_BLOCKED) {
                    status = USERNAME_REQUIRED;
                    strcpy(buff, "Account is blocked!");
                } else if (result == ACCOUNT_NOT_ACTIVE) {
                    status = USERNAME_REQUIRED;
                    strcpy(buff, "Account is not active!");
                }

            } else if (status == VALID_CREDENTIALS) {
                if (strcmp(buff, "bye") == 0) {
                    status = USERNAME_REQUIRED;
                    strcpy(buff, "Goodbye ");
                    strcat(buff, logged_in_user->username);
                    strcat(buff, "!");
                    save_to_file(account_list, FILENAME);
                    logged_in_user = NULL;
                } else {
                    message_status = process_message(buff, digits, letters);
                    if (message_status == 0) {
                        strcpy(buff, "Error!");
                    } else {
                        strcpy(logged_in_user->password, buff);
                    }
                }
            }
        } else {
            status = USERNAME_REQUIRED;
        }


        if (message_status == 0) {
            bytes_sent = sendto(server_sock, buff, strlen(buff), 0, (struct sockaddr *) &server,
                                sizeof(struct sockaddr));
            if (bytes_sent < 0) {
                perror("\nError: ");
                return 0;
            }
        } else {
            bytes_sent = sendto(server_sock, digits, strlen(digits), 0, (struct sockaddr *) &server,
                                sizeof(struct sockaddr));
            if (bytes_sent < 0) {
                perror("\nError: ");
                return 0;
            }
            bytes_sent = sendto(server_sock, letters, strlen(letters), 0, (struct sockaddr *) &server,
                                sizeof(struct sockaddr));
            if (bytes_sent < 0) {
                perror("\nError: ");
                return 0;
            }
        }
    }
}