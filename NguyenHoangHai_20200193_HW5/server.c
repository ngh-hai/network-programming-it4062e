#include <stdio.h>          /* These are the usual header files */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

#define BACKLOG 2   /* Number of allowed connections */
#define BUFF_SIZE 8192

// some client status
#define SEND_MESSAGE 1
#define UPLOAD_FILE 2
#define EXIT 0
#define SELECT_IN_PROGRESS -1

/* a function that extract digit and letter characters from a string
	return 1 if success
	return 0 if given string has non-alphanumeric characters
*/
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

int main(int argc, char *argv[])
{
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
	
	int listen_sock, conn_sock; /* file descriptors */
	char recv_data[BUFF_SIZE]; /* a buffer for sending and receiving data */
	char letters[BUFF_SIZE], digits[BUFF_SIZE]; /* saves the extracted strings */
	int bytes_sent, bytes_received;
	struct sockaddr_in server; /* server's address information */
	struct sockaddr_in client; /* client's address information */
	socklen_t sin_size;
	
	//Step 1: Construct a TCP socket to listen connection request
	if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){  /* create endpoint for connection */
		printf("Error in calling sock");
		return 0;
	}
	
	//Step 2: Bind address to socket
    memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;         
	server.sin_port = htons(PORT);   /* the port to assign */
	server.sin_addr.s_addr = htonl(INADDR_ANY);  /* INADDR_ANY puts your IP address automatically */   
	if(bind(listen_sock, (struct sockaddr*)&server, sizeof(server))==-1){ /* associate with IP address and port number */
		printf("Error in calling bind");
		return 0;
	}     
	
	//Step 3: Listen request from client
	if(listen(listen_sock, BACKLOG) == -1){  /* listen for incoming connections */
		printf("Error in calling listen");
		return 0;
	}
	
	//Step 4: Communicate with client
	while(true){
		//accept request
		sin_size = sizeof(struct sockaddr_in);
		if ((conn_sock = accept(listen_sock,( struct sockaddr *)&client, &sin_size)) == -1)
			printf("Error in calling accept");
  
		printf("New connection from %s\n", inet_ntoa(client.sin_addr) ); /* new client connected */
		
		
        int client_status = SELECT_IN_PROGRESS; // initialize the status of client

		//start conversation
		while(true){
            memset(recv_data,'\0',BUFF_SIZE);

			//receives message from client
			bytes_received = recv(conn_sock, recv_data, BUFF_SIZE, 0); 
			if (bytes_received <= 0){
				printf("Error in receiving data");
				break;
			}
            
            
			recv_data[bytes_received] = '\0';
			printf("\nReceive %d bytes: %s", bytes_received, recv_data);

            if (client_status == SELECT_IN_PROGRESS) {
				recv_data[bytes_received - 1] = '\0';
                if (strcmp(recv_data, "1") == 0) {
                    client_status = SEND_MESSAGE;
                } else if (strcmp(recv_data, "2") == 0) {
                    client_status = UPLOAD_FILE;
                } else if (strcmp(recv_data, "0") == 0) {
                    client_status = EXIT;
					strcpy(recv_data, "Goodbye");
					bytes_sent = send(conn_sock, recv_data, strlen(recv_data), 0);
					if (bytes_sent <= 0){
						printf("Error in receiving data");
					}
					break;
                }
            }
			else if (client_status == SEND_MESSAGE) {
				recv_data[bytes_received - 1] = '\0';
				if (strcmp(recv_data, "") == 0) {
					client_status = SELECT_IN_PROGRESS;
				}
				else if (process_message(recv_data, digits, letters) == 0) {
					strcpy(recv_data, "Invalid message");
					bytes_sent = send(conn_sock, recv_data, strlen(recv_data), 0);
					if (bytes_sent <= 0){
						printf("Error in sending data");
						break;
					}
				}
				else{
					bytes_sent = send(conn_sock, letters, strlen(letters), 0);
					if (bytes_sent < 0){
						printf("Error in sending data");
						break;
					}
					sleep(2); // make sure 2 strings are not concatnated
					bytes_sent = send(conn_sock, digits, strlen(digits), 0);
					if (bytes_sent < 0){
						printf("Error in sending data");
						break;
					}
				}
			}
			else if (client_status == UPLOAD_FILE) {
				if(strcmp(recv_data,"")){
					printf("%s",recv_data);
				}
				else {
					client_status = SELECT_IN_PROGRESS;
				}
			}			
			fflush(stdout);
		}
		// end connection
		close(conn_sock);	
        fflush(stdout);
	}
	close(listen_sock);
	return 0;
}
