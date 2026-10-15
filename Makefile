cc=gcc
WFLAG=-Wall -Wextra -Wno-switch
.PHONY: run debug parser valgrind

sql:main.c
	${cc} -ggdb ${WFLAG} -o sql *.c -I./ -lreadline

run:
	./sql


parser:
	${cc} -U MAIN -D SQLPARSER -ggdb ${WFLAG} -o sql *.c -I./ -lreadline

debug:
	gf2 ./sql

valgrind:
	valgrind --leak-check=full --show-leak-kinds=all ./sql
