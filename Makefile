sql:main.c
	cc -ggdb -Wall -Wextra -o sql *.c -I./

run:sql
	./sql

debug:sql
	gf2 ./sql

parser:
	cc -D SQLPARSER -ggdb -Wall -Wextra -o sql *.c -I./
