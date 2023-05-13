#include "account.h"
#include <stdio.h>          /* These are the usual header files */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#define BACKLOG 20   /* Number of allowed connections */
#define FILENAME "account.txt"

Account account_list = NULL;

/* Receive and echo message to client */
void *echo(void *);

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
    int listenfd, *connfd;
    struct sockaddr_in server; /* server's address information */
    struct sockaddr_in *client; /* client's address information */
    socklen_t sin_size;
    pthread_t tid;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {  /* calls socket() */
        perror("\nError: ");
        return 0;
    }
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = htonl(INADDR_ANY);  /* INADDR_ANY puts your IP address automatically */

    if (bind(listenfd, (struct sockaddr *) &server, sizeof(server)) == -1) {
        perror("\nError: ");
        return 0;
    }

    if (listen(listenfd, BACKLOG) == -1) {
        perror("\nError: ");
        return 0;
    }

    sin_size = sizeof(struct sockaddr_in);
    client = malloc(sin_size);
    while (true) {
        connfd = malloc(sizeof(int));
        if (((*connfd) = accept(listenfd, (struct sockaddr *) client, &sin_size)) == -1) {
            perror("\nError: ");
        }

        printf("You got a connection from %s\n", inet_ntoa(client->sin_addr)); /* prints client's IP */

        /* For each client, spawns a thread, and the thread handles the new client */
        pthread_create(&tid, NULL, &echo, connfd);
    }

    close(listenfd);
    return 0;
}

void *echo(void *arg) {
    int connfd = *((int *) arg);
    free(arg);
    int bytes_sent, bytes_received;
    char buff[MAX_CHARS + 1];
    char username[MAX_CHARS + 1];
    char password[MAX_CHARS + 1];

    pthread_detach(pthread_self());
    int status = USERNAME_REQUIRED;
    while (true) {
        bytes_received = recv(connfd, buff, MAX_CHARS, 0); //blocking
        if (bytes_received == -1) {
            perror("\nError: ");
            break;
        } else if (bytes_received == 0) {
            printf("Connection closed.\n");
            break;
        }
        buff[bytes_received - 1] = '\0';
        printf("Received from client: %s\n", buff);
        switch (status) {
            case USERNAME_REQUIRED: {
                if (strcmp(buff, "") == 0) {
                    strcpy(buff, "goodbye");
                    memset(username, '\0', sizeof(username));
                } else {
                    strcpy(username, buff);
                    strcpy(buff, "enter password");
                    status = PASSWORD_REQUIRED;
                }
                break;
            }
            case PASSWORD_REQUIRED: {
                strcpy(password, buff);
                status = process_login(account_list, username, password);
                switch (status) {
                    case USERNAME_REQUIRED: {
                        strcpy(buff, "username does not exist");
                        memset(username, '\0', sizeof(username));
                        memset(password, '\0', sizeof(password));
                        break;
                    }
                    case VALID_CREDENTIALS: {
                        strcpy(buff, "welcome ");
                        strcat(buff, username);
                        strcat(buff, ", enter \"signout\" to sign out");
                        break;
                    }
                    case WRONG_PASSWORD: {
                        strcpy(buff, "wrong password");
                        status = USERNAME_REQUIRED;
                        memset(username, '\0', sizeof(username));
                        memset(password, '\0', sizeof(password));
                        break;
                    }
                    case ACCOUNT_NOT_ACTIVE: {
                        strcpy(buff, "account is not active");
                        status = USERNAME_REQUIRED;
                        memset(username, '\0', sizeof(username));
                        memset(password, '\0', sizeof(password));
                        break;
                    }
                    case ACCOUNT_BLOCKED: {
                        strcpy(buff, "account is blocked due to 3 failed login attempts");
                        status = USERNAME_REQUIRED;
                        memset(username, '\0', sizeof(username));
                        memset(password, '\0', sizeof(password));
                        break;
                    }
                }
                break;
            }
            case VALID_CREDENTIALS: {
                if (strcmp(buff, "signout") == 0) {
                    strcpy(buff, "you have signed out, ");
                    strcat(buff, username);
                    status = USERNAME_REQUIRED;
                    memset(username, '\0', sizeof(username));
                    memset(password, '\0', sizeof(password));
                } else {
                    strcpy(buff, "enter \"signout\" to sign out");
                }
                break;
            }
        }

        bytes_sent = send(connfd, buff, strlen(buff), 0); /* reply to client with proper messages */
        if (bytes_sent == -1) {
            perror("\nError: ");
            break;
        }
    }
    close(connfd);
    save_to_file(account_list, FILENAME); // save any updates to file
    return NULL;
}
