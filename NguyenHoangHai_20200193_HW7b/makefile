CC = gcc
CFLAGS = -Wall -Werror

all: server client

server: server.o account.o
	$(CC) $(CFLAGS) server.o account.o -o server

client: client.o
	$(CC) $(CFLAGS) client.o -o client

server.o: server.c
	$(CC) $(CFLAGS) server.c -o server.o -c

client.o: client.c
	$(CC) $(CFLAGS) client.c -o client.o -c

account.o: account.c
	$(CC) $(CFLAGS) account.c -o account.o -c

clean:
	rm -f server client *.o