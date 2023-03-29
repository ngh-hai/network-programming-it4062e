ifeq ($(OS),Windows_NT)
    CC = gcc
    RM = del /Q
    EXECUTABLE = NguyenHoangHai_20200193_HW1.exe
else
    CC = gcc
    RM = rm -f
    EXECUTABLE = NguyenHoangHai_20200193_HW1
endif
CFLAGS = -Wall -Werror

all: $(EXECUTABLE)
	./$(EXECUTABLE)

$(EXECUTABLE): NguyenHoangHai_20200193_HW1.c
	$(CC) $(CFLAGS) NguyenHoangHai_20200193_HW1.c -o $(EXECUTABLE)

clean:
	$(RM) $(EXECUTABLE)
