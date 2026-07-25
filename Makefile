sql:main.c
	cc -Wall -Wextra -o sql *.c -I./

run:debug
	./sql

debug:main.c
	cc -ggdb -Wall -Wextra -o sql *.c -I./
