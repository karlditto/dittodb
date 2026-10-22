cc=gcc
WFLAG=-Wall -Wextra -Wno-unused-parameter -Wno-unused-local-typedefs
Headers=*.h
.PHONY: run debug parser valgrind

sql:main.c ${Headers}
	${cc} -ggdb ${WFLAG} -o sql *.c -I./ -lreadline

run:
	./sql


parser:
	${cc} -U MAIN -D SQLPARSER -ggdb ${WFLAG} -o sql *.c -I./ -lreadline

debug:
	gf2 ./sql

valgrind:
	valgrind --leak-check=full --show-leak-kinds=all ./sql
