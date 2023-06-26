CC = gcc
CFLAGS = -Wall -Werror

all: server client

server: server.o
	$(CC) $(CFLAGS) server.o -o server

client: client.o
	$(CC) $(CFLAGS) client.o -o client

server.o: server.c
	$(CC) $(CFLAGS) server.c -o server.o -c

client.o: client.c
	$(CC) $(CFLAGS) client.c -o client.o -c

clean:
	rm -f server client *.o