#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <stdbool.h>

#define BUFF_SIZE 1024

struct thread_args {
    int client_sock;
    struct sockaddr_in server_addr;
};

bool is_valid_ipv4_address(const char *); // check if the IP address is valid

// 2 threads to send and receive messages concurrently
void *recv_thread(void *);

void *send_thread(void *);

int main(int argc, char *argv[]) {
    // check if the number of arguments is correct
    if (argc != 3) {
        printf("Usage: ./client <ip_address> <port_number>\n");
        return 0;
    }
    // check if the ip address is valid
    char *SERV_IP = argv[1];
    if (!is_valid_ipv4_address(SERV_IP)) {
        printf("Error: invalid IP address\n");
        return 0;
    }
    // check if the port number is valid
    char *buff = (char *) malloc(BUFF_SIZE * sizeof(char));
    errno = 0;
    uint16_t SERV_PORT = strtoul(argv[2], &buff, 10);
    if (errno != 0 || *buff != '\0') {
        printf("Error: %s\n", errno == EINVAL ? "invalid base" : "invalid input");
        return 0;
    }

    int client_sock;
    struct sockaddr_in server_addr;
    pthread_t thread_send, thread_recv;

    //Step 1: Construct a UDP socket
    if ((client_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {  /* calls socket() */
        perror("\nError: ");
        exit(0);
    }

    //Step 2: Define the address of the server
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERV_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERV_IP);

    // create a struct as an argument to threads
    struct thread_args args;
    args.client_sock = client_sock;
    args.server_addr = server_addr;
    // Create threads for sending and receiving data
    if (pthread_create(&thread_send, NULL, send_thread, (void *) &args) != 0) {
        perror("pthread_create for send_thread");
        return 0;
    }
    if (pthread_create(&thread_recv, NULL, recv_thread, (void *) &args) != 0) {
        perror("pthread_create for recv_thread");
        return 0;
    }
    // Wait for threads to finish
    pthread_join(thread_send, NULL);
    pthread_join(thread_recv, NULL);
    return 0;
}

void *recv_thread(void *_args) {
    struct thread_args *args = (struct thread_args *) _args;
    int client_sock = args->client_sock;
    struct sockaddr_in server_addr = args->server_addr;
    char *buff = (char *) malloc(BUFF_SIZE * sizeof(char));
    socklen_t sin_size = sizeof(struct sockaddr);
    ssize_t bytes_received;
    while (true) {
        if (client_sock == -1 ||
            getsockopt(client_sock, SOL_SOCKET, SO_ERROR, &(int) {0}, &(socklen_t) {sizeof(int)}) != 0) {
            recvfrom(client_sock, buff, BUFF_SIZE - 1, 0, (struct sockaddr *) &server_addr, &sin_size);
            break; // socket is closed
        } else {
            memset(buff, '\0', BUFF_SIZE);
            bytes_received = recvfrom(client_sock, buff, BUFF_SIZE - 1, 0, (struct sockaddr *) &server_addr, &sin_size);
            if (bytes_received == -1) {
                perror("Error: ");
                close(client_sock);
                break;
            }
            // printf("\nReply from server: %s\n", buff);
            printf("\n%s\n", buff);
        }
    }
    pthread_exit(NULL);
}

void *send_thread(void *_args) {
    struct thread_args *args = (struct thread_args *) _args;
    int client_sock = args->client_sock;
    struct sockaddr_in server_addr = args->server_addr;
    char *buff = (char *) calloc(BUFF_SIZE, sizeof(char));
    socklen_t sin_size = sizeof(struct sockaddr);
    ssize_t bytes_send;
    do {
        // printf("\nEnter a message:\n");
        fgets(buff, BUFF_SIZE, stdin);
        buff[strlen(buff) - 1] = '\0';
        bytes_send = sendto(client_sock, buff, strlen(buff), 0, (struct sockaddr *) &server_addr, sin_size);
        if (bytes_send == -1) {
            perror("Error: ");
            close(client_sock);
            break;
        }
    } while (strcmp(buff, "") != 0); // send an empty message to exit
    close(client_sock);
    pthread_exit(NULL);
}

bool is_valid_ipv4_address(const char *str) {
    struct in_addr addr;
    return inet_pton(AF_INET, str, &addr) == 1;
}