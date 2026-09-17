sql:main.c
	cc -ggdb -Wall -Wextra -o sql *.c -I./

run:
	cc -ggdb -Wall -Wextra -o sql *.c -I./ && ./sql

debug:
	gf2 ./sql

parser:
	cc -U MAIN -D SQLPARSER -ggdb -Wall -Wextra -o sql *.c -I./
