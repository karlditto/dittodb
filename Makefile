sql:main.c
	cc -Wall -Wextra -o sql *.c -I./

run:sql
	./sql

debug:main.c
	cc -g -Wall -Wextra -o sql *.c -I./
