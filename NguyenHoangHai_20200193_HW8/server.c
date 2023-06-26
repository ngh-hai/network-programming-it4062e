#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

const int MAX_CLIENTS = 5;
const int MAX_BUFFER_SIZE = 1024;

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: ./server <port_number>\n");
        return 0;
    }
    char *endstr;
    // check if the port number is valid
    errno = 0;
    uint16_t PORT = strtoul(argv[1], &endstr, 10);

    if (errno != 0 || *endstr != '\0')
    {
        printf("Error: %s\n", errno == EINVAL ? "invalid base" : "invalid input");
        return 0;
    }
    int server_socket, client_sockets[MAX_CLIENTS], client_ports[MAX_CLIENTS];
    char client_addr[MAX_CLIENTS][MAX_BUFFER_SIZE];
    char filename[MAX_BUFFER_SIZE];
    struct sockaddr_in server_address, client_address;
    socklen_t addrlen = sizeof(client_address);
    int connected_clients = 0;
    fd_set read_fds, temp_fds;
    int max_fd;

    // Create a TCP socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Set the socket to non-blocking mode
    int flags = fcntl(server_socket, F_GETFL, 0);
    fcntl(server_socket, F_SETFL, flags | O_NONBLOCK);

    // Bind the socket to a specific address and port
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);
    bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address));

    // Listen for incoming connections
    listen(server_socket, MAX_CLIENTS);

    printf("Server started on port %d\n", PORT);

    // Clear the file descriptor sets
    FD_ZERO(&read_fds);
    FD_ZERO(&temp_fds);

    // Add the server socket to the set
    FD_SET(server_socket, &read_fds);
    max_fd = server_socket;

    while (true)
    {
        temp_fds = read_fds;

        // Use select to monitor the file descriptors for readability
        int activity = select(max_fd + 1, &temp_fds, NULL, NULL, NULL);

        if (activity == -1)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // Check if the server socket has activity
        if (FD_ISSET(server_socket, &temp_fds))
        {
            // Accept new client connections
            int client_socket = accept(server_socket, (struct sockaddr *)&client_address, &addrlen);

            if (client_socket >= 0)
            {
                // Set the client socket to non-blocking mode
                int flags = fcntl(client_socket, F_GETFL, 0);
                fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);

                client_sockets[connected_clients] = client_socket;
                client_ports[connected_clients] = ntohs(client_address.sin_port);
                strcpy(client_addr[connected_clients], inet_ntoa(client_address.sin_addr));

                // Check if file exists
                strcpy(filename, client_addr[connected_clients]);
                strcat(filename, "_");
                char port[10];
                sprintf(port, "%d", client_ports[connected_clients]);
                strcat(filename, port);
                strcat(filename, ".txt");
                printf("%s", filename);
                // if filename exists, delete it
                if (access(filename, F_OK) == 0)
                {
                    remove(filename);
                }

                connected_clients++;
                printf("New client connected: %s[%d]\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

                // Add the new client socket to the set
                FD_SET(client_socket, &read_fds);

                // Update the maximum file descriptor
                if (client_socket > max_fd)
                {
                    max_fd = client_socket;
                }
            }
            // If there are no more file descriptors with activity, continue
            if (--activity <= 0)
            {
                continue;
            }
        }
        // Process data from connected clients
        struct utsname uname_pointer;
        int i;
        for (i = 0; i < connected_clients; i++)
        {
            int client_fd = client_sockets[i];
            if (FD_ISSET(client_fd, &temp_fds))
            {
                int bytes_received = recv(client_fd, &uname_pointer, sizeof(struct utsname), 0);
                if (bytes_received > 0)
                {
                    printf("Received data from client ");
                    printf("%s", client_addr[i]);
                    printf("[%d]\n", client_ports[i]);
                    // assume client always send uname successfully
                    strcpy(filename, client_addr[i]);
                    strcat(filename, "_");
                    char port[10];
                    sprintf(port, "%d", client_ports[i]);
                    strcat(filename, port);
                    strcat(filename, ".txt");
                    FILE *f = fopen(filename, "a+");
                    if (f == NULL)
                    {
                        printf("Error opening file!\n");
                        exit(1);
                    }
                    fprintf(f, "IP address: %s\n", client_addr[i]);
                    fprintf(f, "Port number: %d\n", client_ports[i]);
                    fprintf(f, "System name: %s\n", uname_pointer.sysname);
                    fprintf(f, "Node name: %s\n", uname_pointer.nodename);
                    fprintf(f, "Release: %s\n", uname_pointer.release);
                    fprintf(f, "Version: %s\n", uname_pointer.version);
                    fprintf(f, "Machine: %s\n", uname_pointer.machine);
                    fclose(f);
                    printf("uname data saved to file %s\n", filename);
                }
                else if (bytes_received == 0)
                {
                    // Client disconnected
                    close(client_fd);
                    FD_CLR(client_fd, &read_fds);
                    connected_clients--;
                    client_sockets[i] = client_sockets[connected_clients];
                    client_ports[i] = client_ports[connected_clients];
                    strcpy(client_addr[i], client_addr[connected_clients]);
                    printf("Client disconnected\n");
                }
                else if (errno != EWOULDBLOCK && errno != EAGAIN)
                {
                    // Error occurred
                    fprintf(stderr, "Error receiving data from client: %s\n", strerror(errno));
                }
                // If there are no more file descriptors with activity, break
                if (--activity <= 0)
                {
                    break;
                }
            }
        }
    }
    return 0;
}
