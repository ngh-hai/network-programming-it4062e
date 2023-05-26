#include "account.h"
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

const int BACKLOG = 20;
const char *FILENAME = "account.txt";

pthread_mutex_t lock;

Account account_list = NULL;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./server <port_number>\n");
        return 0;
    }
    char *endstr;
    // check if the port number is valid
    errno = 0;
    uint16_t PORT = strtoul(argv[1], &endstr, 10);

    if (errno != 0 || *endstr != '\0') {
        printf("Error: %s\n", errno == EINVAL ? "invalid base" : "invalid input");
        return 0;
    }

    account_list = read_account(FILENAME);

    // Construct socket
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        perror("Error: ");
        return 0;
    }
    // Bind address to socket
    struct sockaddr_in server;
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenfd, (struct sockaddr *) &server, sizeof(server)) == -1) {
        perror("Error: ");
        return 0;
    }
    // Listen to requests
    if (listen(listenfd, BACKLOG) == -1) {
        perror("Error: ");
        return 0;
    }

    int client[FD_SETSIZE], clientStatus[FD_SETSIZE], connfd;
    fd_set checkfds, readfds;
    struct sockaddr_in clientAddr;
    int nEvents;
    socklen_t clientAddrLen;
    char rcvBuff[MAX_CHARS], sendBuff[MAX_CHARS];
    char *username[FD_SETSIZE], *password[FD_SETSIZE];

    // Initialize data structures
    for (int i = 0; i < FD_SETSIZE; ++i) {
        client[i] = -1;
        clientStatus[i] = -1;
        username[i] = NULL;
        password[i] = NULL;
    }

    FD_ZERO(&checkfds);
    FD_SET(listenfd, &checkfds);
    int maxfd = listenfd;

    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\n Mutex init has failed\n");
        return 1;
    }

    // Accept connection and communicate with clients
    while (true) {
        int i;
        readfds = checkfds;
        nEvents = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (nEvents == -1) {
            perror("Error: ");
            break;
        } else if (nEvents == 0) {
            printf("Timeout.\n");
            break;
        }
        if (FD_ISSET(listenfd, &readfds)) {
            clientAddrLen = sizeof(clientAddr);
            connfd = accept(listenfd, (struct sockaddr *) (&clientAddr), &clientAddrLen);
            printf("New accepted connfd: %d\n", connfd);
            for (i = 0; i < FD_SETSIZE; ++i) {
                if (client[i] <= 0) {
                    client[i] = connfd;
                    clientStatus[i] = USERNAME_REQUIRED;
                    username[i] = (char *) malloc(sizeof(char) * MAX_CHARS);
                    password[i] = (char *) malloc(sizeof(char) * MAX_CHARS);
                    FD_SET(client[i], &checkfds);
                    if (connfd > maxfd) maxfd = connfd;
                    break;
                }
                if (--nEvents <= 0) continue;
            }
            if (i == FD_SETSIZE) {
                printf("Max number of clients reached.");
            }
        }
        // check status of connfd(s)
        for (i = 0; i < FD_SETSIZE; ++i) {
            if (client[i] <= 0) continue;
            if (FD_ISSET(client[i], &readfds)) {
                int hasClosed = 0;
                memset(rcvBuff, '\0', MAX_CHARS);
                memset(sendBuff, '\0', MAX_CHARS);
                int bytes_received = recv(client[i], rcvBuff, MAX_CHARS, 0);
                if (bytes_received <= 0) {
                    printf("Connection closed.\n");
                    FD_CLR(client[i], &checkfds);
                    close(client[i]);
                    client[i] = -1;
                    clientStatus[i] = -1;
                    free(username[i]);
                    free(password[i]);
                    username[i] = NULL;
                    password[i] = NULL;
                    hasClosed = 1;
                } else {
                    rcvBuff[bytes_received - 1] = '\0';
                    printf("Received from fd %d: %s\n", client[i], rcvBuff);
                    switch (clientStatus[i]) {
                        case USERNAME_REQUIRED: {
                            if (strcmp(rcvBuff, "") == 0) {
                                strcpy(sendBuff, "goodbye");
                                memset(rcvBuff, '\0', MAX_CHARS);
                            } else {
                                strcpy(username[i], rcvBuff);
                                strcpy(sendBuff, "enter password: ");
                                clientStatus[i] = PASSWORD_REQUIRED;
                            }
                            break;
                        }
                        case PASSWORD_REQUIRED: {
                            strcpy(password[i], rcvBuff);
                            clientStatus[i] = process_login(account_list, username[i], password[i]);
                            switch (clientStatus[i]) {
                                case USERNAME_REQUIRED: {
                                    strcpy(sendBuff, "username does not exist");
                                    memset(username[i], '\0', MAX_CHARS);
                                    memset(password[i], '\0', MAX_CHARS);
                                    break;
                                }
                                case ACCOUNT_ALREADY_SIGNED_IN: {
                                    clientStatus[i] = USERNAME_REQUIRED;
                                    strcpy(sendBuff, "account is already signed in");
                                    memset(username[i], '\0', MAX_CHARS);
                                    memset(password[i], '\0', MAX_CHARS);
                                    break;
                                }
                                case VALID_CREDENTIALS: {
                                    strcpy(sendBuff, "welcome ");
                                    strcat(sendBuff, username[i]);
                                    strcat(sendBuff, ", enter \"signout\" to sign out");
                                    break;
                                }
                                case WRONG_PASSWORD: {
                                    clientStatus[i] = USERNAME_REQUIRED;
                                    strcpy(sendBuff, "wrong password");
                                    memset(username[i], '\0', MAX_CHARS);
                                    memset(password[i], '\0', MAX_CHARS);
                                    break;
                                }
                                case ACCOUNT_NOT_ACTIVE: {
                                    clientStatus[i] = USERNAME_REQUIRED;
                                    strcpy(sendBuff, "account is not active");
                                    memset(username[i], '\0', MAX_CHARS);
                                    memset(password[i], '\0', MAX_CHARS);
                                    break;
                                }
                                case ACCOUNT_BLOCKED: {
                                    clientStatus[i] = USERNAME_REQUIRED;
                                    strcpy(sendBuff, "account is blocked due to 3 failed login attempts");
                                    memset(username[i], '\0', MAX_CHARS);
                                    memset(password[i], '\0', MAX_CHARS);
                                    break;
                                }
                            }
                            break;
                        }
                        case VALID_CREDENTIALS: {
                            if (strcmp(rcvBuff, "signout") == 0) {
                                clientStatus[i] = USERNAME_REQUIRED;
                                process_logout(account_list, username[i]);
                                strcpy(sendBuff, "signed out");
                                memset(username[i], '\0', MAX_CHARS);
                                memset(password[i], '\0', MAX_CHARS);
                            } else {
                                strcpy(sendBuff, "enter \"signout\" to sign out");
                            }
                            break;
                        }
                    }
                    int bytes_sent = send(client[i], sendBuff, strlen(sendBuff), 0);
                    if (bytes_sent <= 0) {
                        printf("Connection closed.\n");
                        FD_CLR(client[i], &checkfds);
                        close(client[i]);
                        client[i] = -1;
                        clientStatus[i] = -1;
                        free(username[i]);
                        free(password[i]);
                        username[i] = NULL;
                        password[i] = NULL;
                        hasClosed = 1;
                    }
                }
                if (hasClosed) {
                    pthread_mutex_lock(&lock);
                    save_to_file(account_list, FILENAME);
                    pthread_mutex_unlock(&lock);
                }
                if (--nEvents <= 0) continue;
            }
        }
    }
    pthread_mutex_destroy(&lock);
    close(listenfd);
    printf("Server closed.\n");
    return 0;
}
