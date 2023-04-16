CC = gcc
RM = rm -f
EXECUTABLE = resolver
CFLAGS = -Wall -Werror

all: $(EXECUTABLE)

$(EXECUTABLE): resolver.o
	$(CC) $(CFLAGS) -o $(EXECUTABLE) resolver.o

resolver.o: resolver.c
	$(CC) $(CFLAGS) -c resolver.c

clean:
	$(RM) $(EXECUTABLE) resolver.o
