#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>
const int BUFF_SIZE = 8192;

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
	int client_sock;
	struct sockaddr_in server_addr; /* server's address information */
	int bytes_sent;

	// Step 1: Construct socket
	client_sock = socket(AF_INET, SOCK_STREAM, 0);

	// Step 2: Specify server address
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERV_PORT);
	server_addr.sin_addr.s_addr = inet_addr(SERV_IP);

	// Step 3: Request to connect server
	if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) < 0)
	{
		printf("\nError!Can not connect to sever! Client exit immediately! ");
		return 0;
	}

	// Step 4: Use uname() to get system information

	struct utsname uname_pointer;
	uname(&uname_pointer);
	bytes_sent = send(client_sock, &uname_pointer, sizeof(struct utsname), 0);
	if (bytes_sent <= 0)
	{
		printf("\nConnection closed!\n");
	}
	// Step 5: Close socket
	close(client_sock);
	return 0;
}
