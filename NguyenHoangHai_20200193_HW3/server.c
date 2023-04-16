#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

#define BUFF_SIZE 1024
#define MAX_CLIENTS 2

int process_message(const char *, char *, char *);

int main(int argc, char *argv[]) {
    // check if the number of arguments is correct
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

    int server_sock; /* file descriptors */
    ssize_t bytes_sent, bytes_received;
    struct sockaddr_in server; /* server's address information */
    struct sockaddr_in client_tmp; /* temporary variable of client address information */
    struct sockaddr_in clients[MAX_CLIENTS]; /* a list of clients */
    socklen_t clients_size[MAX_CLIENTS]; /* a list of clients' size */
    int sin_size;
    char *digits = (char *) malloc(BUFF_SIZE * sizeof(char)); /* a buffer to store digits */
    char *letters = (char *) malloc(BUFF_SIZE * sizeof(char));  /* a buffer to store letters */

    //Step 1: Construct a UDP socket
    server_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_sock == -1) {  // socket failed
        perror("\nError: ");
        return 0;
    }

    //Step 2: Bind address to socket
    server.sin_family = AF_INET; /* IPv4 */
    server.sin_port = htons(PORT);   /* port */
    server.sin_addr.s_addr = INADDR_ANY;  /* listen to any interface */
    bzero(&(server.sin_zero), 8); /* flush the rest of struct */

    int bind_status = bind(server_sock, (struct sockaddr *) &server, sizeof(struct sockaddr)); /* calls bind() */
    if (bind_status == -1) { // bind failed
        perror("\nError: ");
        return 0;
    }

    //Step 3: Communicate with clients
    while (true) {
        memset(buff, '\0', BUFF_SIZE);
        memset(digits, '\0', BUFF_SIZE);
        memset(letters, '\0', BUFF_SIZE);

        sin_size = sizeof(struct sockaddr_in);
        bytes_received = recvfrom(server_sock, buff, BUFF_SIZE - 1, 0, (struct sockaddr *) &client_tmp,
                                  (socklen_t *) (&sin_size)); // receive message and store the client_tmp's address
        // client_tmp can be a new client or an existing client
        int idx = 0;
        if (bytes_received == -1) {
            perror("\nError: ");
            close(server_sock);
            return 0;
        } else {
            // check if the client_tmp is already in the list
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
            printf("Received from [%s:%d]: %s\n", inet_ntoa(client_tmp.sin_addr), ntohs(client_tmp.sin_port), buff);
        }
        if (strcmp(buff, "") == 0) { // if the client_tmp sent an empty message, skip it -> maybe the client has exited
            sendto(server_sock, buff, strlen(buff), 0, (struct sockaddr *) &client_tmp, sin_size);
        } else if (clients[MAX_CLIENTS - idx - 1].sin_addr.s_addr == 0 &&
                   clients[MAX_CLIENTS - idx - 1].sin_port == 0) {
            continue; // if the client_tmp is the only one in the list, skip it
        } else if (process_message(buff, digits, letters) == 0) {
            sendto(server_sock, "Error", strlen("Error"), 0, (struct sockaddr *) &client_tmp,
                   sin_size); /* the message is invalid */
            if (bytes_sent == -1) {
                perror("\nError: ");
                close(server_sock);
                return 0;
            }
        } else {
            bytes_sent = sendto(server_sock, letters, strlen(letters), 0,
                                (struct sockaddr *) &clients[MAX_CLIENTS - idx - 1],
                                clients_size[MAX_CLIENTS - idx - 1]); /* send to the client_tmp string of letters */
            if (bytes_sent == -1) {
                perror("\nError: ");
                close(server_sock);
                return 0;
            }
            bytes_sent = sendto(server_sock, digits, strlen(digits), 0,
                                (struct sockaddr *) &clients[MAX_CLIENTS - idx - 1],
                                clients_size[MAX_CLIENTS - idx - 1]); /* send to the client_tmp string of digits */
            if (bytes_sent == -1) {
                perror("\nError: ");
                close(server_sock);
                return 0;
            }
        }
    }
}

// process the message
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