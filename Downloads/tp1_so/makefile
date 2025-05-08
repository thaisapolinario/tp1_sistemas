EXEC = shell

HDR_DIR = include

CC = gcc
CFLAGS = -Wall -Wextra -g -I$(HDR_DIR)

SRCS = shell.c
OBJS = shell.o

all: $(EXEC)

#biblioteca readline
$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lreadline


shell.o: shell.c
	$(CC) $(CFLAGS) -c shell.c -o shell.o


clean:
	rm -f $(OBJS) $(EXEC)


run: all
	./$(EXEC)
