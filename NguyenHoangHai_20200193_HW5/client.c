#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <errno.h>

#define BUFF_SIZE 8192

#define SEND_MESSAGE 1
#define UPLOAD_FILE 2
#define EXIT 0
#define SELECT_IN_PROGRESS -1

/* check if the given string is valid IPv4 address */
bool is_valid_ipv4_address(const char *str) {
    struct in_addr addr;
    return inet_pton(AF_INET, str, &addr) == 1;
}

void menu() {
	printf("\n===========MENU============\n");
	printf("1. Send message\n");
	printf("2. Upload file\n\n");
	printf("0. Exit\n");
	printf("=======================\n");
	printf("---> Your choice: ");
}

/* a thread for receiving data*/
void *recv_thread(void *arg) {
    int client_sock = *((int*)arg);
    char recv_msg[BUFF_SIZE];
    
    while(true) {
        memset(recv_msg, '\0', BUFF_SIZE);
        int bytes_received = recv(client_sock, recv_msg, BUFF_SIZE, 0); // BUFF_SIZE or BUFF_SIZE - 1?
        if(bytes_received <= 0) {
            printf("\nError! Cannot receive data from sever!\n");
            break;
        }
        recv_msg[bytes_received] = '\0';
        printf("\n\nReply from server: %s\n", recv_msg);
        fflush(stdout);
    }
    close(client_sock);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]){
	// check if the number of arguments is correct
    if (argc != 3) {
        printf("Usage: ./client <ip_address> <port_number>\n");
        return 0;
    }
    // check if the IP address is valid
    char *SERV_IP = argv[1];
    if (!is_valid_ipv4_address(SERV_IP)) {
        printf("Error: invalid IP address\n");
        return 0;
    }
    // check if the port number is valid
    char *endptr;
    errno = 0;
    uint16_t SERV_PORT = strtoul(argv[2], &endptr, 10);
    if (errno != 0 || *endptr != '\0') {
        printf("Error: %s\n", errno == EINVAL ? "invalid base" : "invalid input");
        return 0;
    }

	int client_sock;
	char buff[BUFF_SIZE];
	struct sockaddr_in server_addr; /* server's address information */
	int bytes_sent;
	
	//Step 1: Construct socket
	client_sock = socket(AF_INET,SOCK_STREAM,0);
	
	//Step 2: Specify server address
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERV_PORT);
	server_addr.sin_addr.s_addr = inet_addr(SERV_IP);
	
	//Step 3: Request to connect server
	if(connect(client_sock, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) == -1){
		printf("\nError!Can not connect to sever! Client exit immediately! ");
		return 0;
	}

    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); // receiving is independent task

    pthread_create(&thread, &attr, recv_thread, (void*)(&client_sock));
		
	//Step 4: Communicate with server		

	int client_status = SELECT_IN_PROGRESS;
	int choice = -1;

	while(true){
		memset(buff,'\0',BUFF_SIZE);
		//send message
		if (client_status == SELECT_IN_PROGRESS) {
			while(true){
				menu();
				memset(buff,'\0',BUFF_SIZE);
				fgets(buff, BUFF_SIZE, stdin);
				buff[strlen(buff) - 1] = '\0';
				choice = strtol(buff, &endptr, 10);
				if (endptr == buff || *endptr != '\0') {
					printf("Error: invalid input\n");
					continue;
				}
				else if (choice != SEND_MESSAGE && choice != UPLOAD_FILE && choice != EXIT) {
					printf("Error: invalid choice\n");
					continue;
				}
				else {
					break;
				}
			}
			if (choice == EXIT) {
				strcpy(buff, "0\n");
				client_status = EXIT;
			}
			else if (choice == SEND_MESSAGE) {
				strcpy(buff, "1\n");
				client_status = SEND_MESSAGE;
			}
			else if (choice == UPLOAD_FILE) {
				strcpy(buff, "2\n");
				client_status = UPLOAD_FILE;
			}
			bytes_sent = send(client_sock, buff, strlen(buff), 0);
			if(bytes_sent <= 0){
				printf("Error in sending data");
				break;
			}
		}
		else if (client_status == SEND_MESSAGE) {
			do{
				printf("\nInsert string to send:");
				fgets(buff, BUFF_SIZE, stdin);
				if (strcmp(buff, "\n") == 0) { // finish sending, return to main menu
					client_status = SELECT_IN_PROGRESS;
					choice = -1;
				}
				bytes_sent = send(client_sock, buff, strlen(buff), 0);
				if(bytes_sent <= 0){
					printf("Error in sending data");
					break;
				}
			} while (strcmp(buff, "\n") != 0);
		}

		else if (client_status == UPLOAD_FILE) {
			printf("\nEnter file name: ");
			fgets(buff, BUFF_SIZE, stdin);
			
			buff[strlen(buff) - 1] = '\0';
			FILE *f = fopen(buff, "r");
			if (f == NULL) {
				perror("fopen");
				client_status = SELECT_IN_PROGRESS;
				choice = -1;
				strcpy(buff, "");
				bytes_sent = send(client_sock, buff, 1, 0);
				if(bytes_sent <= 0){
					printf("Error in sending data");
					break;
				}
				continue;
			}

			int nread = 0;
			while((nread = fread(buff, sizeof(char), BUFF_SIZE, f)) > 0) {
				bytes_sent = send(client_sock, buff, nread, 0);
				if(bytes_sent <= 0){
					printf("Error in sending data");
					break;
				}
			}
			sleep(2); // make sure EOF signal does not be concatnated to the file content.
			// EOF
			strcpy(buff, "");
			bytes_sent = send(client_sock, buff, 1, 0);
			if(bytes_sent <= 0){
				printf("Error in sending data");
				break;
			}

			fclose(f);
			client_status = SELECT_IN_PROGRESS;
			choice = -1;
		}

		else { // client wants to exit
			choice = -1;
			break;
		}
        fflush(stdout);
	}
	//Step 4: Close socket
	close(client_sock);
	return 0;
}
