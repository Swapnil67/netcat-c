CFLAGS=-Wall -Wextra -Wconversion -Werror -pedantic --std=c17

main: main.c
	$(CC) $(CFLAGS) -o main main.c
