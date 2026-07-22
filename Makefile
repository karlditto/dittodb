sql:main.c
	cc -Wall -Wextra -o sql *.c -I./

run:sql
	./sql
