#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

#define NOT_SIGNED_IN 0
#define SIGNED_IN 1
#define SEND_MESSAGE 2
#define UPLOAD_IMAGE 3
#define BUFF_SIZE 32768

bool is_valid_ipv4_address(const char *str)
{
    struct in_addr addr;
    return inet_pton(AF_INET, str, &addr) == 1;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: ./client <ip_address> <port_number>\n");
        return 0;
    }
    // check if the IP address is valid
    char *SERV_IP = argv[1];
    if (!is_valid_ipv4_address(SERV_IP))
    {
        printf("Error: invalid IP address\n");
        return 0;
    }
    // check if the port number is valid
    char *endptr;
    errno = 0;
    uint16_t SERV_PORT = strtoul(argv[2], &endptr, 10);
    if (errno != 0 || *endptr != '\0')
    {
        printf("Error: %s\n", errno == EINVAL ? "invalid base" : "invalid input");
        return 0;
    }

    char buff[BUFF_SIZE + 1];
    int client_sock;
    struct sockaddr_in server_addr; /* server's address information */
    int msg_len, bytes_sent, bytes_received;

    // Step 1: Construct socket
    client_sock = socket(AF_INET, SOCK_STREAM, 0);

    // Step 2: Specify server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERV_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERV_IP);

    // Step 3: Request to connect server
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
    {
        printf("\nError!Can not connect to sever! Client exit immediately! ");
        return 0;
    }

    int status = NOT_SIGNED_IN;
    int has_signed_out = 0;

    // Step 4: Communicate with server
    while (true)
    {
        printf("CURRENT STATUS: %d\n", status);
        // send message
        printf("\nInsert string to send:");
        memset(buff, '\0', BUFF_SIZE);
        fgets(buff, BUFF_SIZE, stdin);
        msg_len = strlen(buff);
        if (msg_len == 0)
            break;

        if (status == UPLOAD_IMAGE)
        {
            if (strcmp(buff, "\n"))
            {
                // remove \n
                buff[strlen(buff) - 1] = '\0';
                // open file, read and send
                FILE *fp = fopen(buff, "rb");
                if (fp == NULL)
                {
                    printf("Error: cannot open file\n");
                    continue;
                }
                // append \n
                buff[strlen(buff)] = '\n';
                bytes_sent = send(client_sock, buff, msg_len, 0);
                if (bytes_sent == -1)
                    perror("\nError: ");
                printf("Filename sent\n");
                printf("Sending file...\n");
                int nread;

                while ((nread = fread(buff, 1, BUFF_SIZE, fp)) > 0)
                {
                    // sleep(1.0);
                    printf("read %d bytes\n", nread);
                    bytes_sent = send(client_sock, buff, nread, 0);
                    printf("sent %d bytes\n", bytes_sent);
                    if (bytes_sent <= 0)
                    {
                        printf("Error in sending data1");
                        break;
                    }
                }

                sleep(2);

                memset(buff, '\0', BUFF_SIZE);
                bytes_sent = send(client_sock, buff, 1, 0);
                if (bytes_sent <= 0)
                {
                    printf("Error in sending data2");
                    break;
                }

                fclose(fp);
            }
            else
            {
                bytes_sent = send(client_sock, buff, msg_len, 0);
                if (bytes_sent == -1)
                    perror("\nError: ");
            }
        }
        else
        {
            bytes_sent = send(client_sock, buff, msg_len, 0);
            if (bytes_sent == -1)
                perror("\nError: ");
        }

        // receive echo reply
        bytes_received = recv(client_sock, buff, BUFF_SIZE, 0);
        if (bytes_received == -1)
            perror("\nError: ");
        else if (bytes_received == 0)
            printf("Connection closed.\n");

        buff[bytes_received] = '\0';
        printf("Reply from server:\n%s", buff);
        switch (status)
        {
        case NOT_SIGNED_IN:
        {
            if (strcmp(buff, "goodbye") == 0)
            {
                has_signed_out = 1;
            }
            else if (strstr(buff, "welcome"))
            {
                status = SIGNED_IN;
            }
            break;
        }
        case SIGNED_IN:
        {
            if (strstr(buff, "signed out"))
            {
                status = NOT_SIGNED_IN;
            }
            else if (strstr(buff, "Entered send message mode"))
            {
                status = SEND_MESSAGE;
            }
            else if (strstr(buff, "Entered upload image mode"))
            {
                status = UPLOAD_IMAGE;
            }
            break;
        }
        case SEND_MESSAGE:
        {
            if (strstr(buff, "Exited send message mode"))
            {
                status = SIGNED_IN;
            }
            break;
        }
        case UPLOAD_IMAGE:
        {
            if (strstr(buff, "Exited upload image mode"))
            {
                status = SIGNED_IN;
            }
            break;
        }
        }
        if (has_signed_out)
        {
            break;
        }
    }
    printf("Client stopped!\n");
    // Step 4: Close socket
    close(client_sock);
    return 0;
}
