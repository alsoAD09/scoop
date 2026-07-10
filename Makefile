all: scoop

scoop: scoop.c clean_print.c get_syscall.c
	gcc -Wall -Wextra -g scoop.c clean_print.c get_syscall.c -o scoop

install: scoop
	sudo cp scoop /usr/local/bin/scoop

clean:
	rm -f scoop *.o
