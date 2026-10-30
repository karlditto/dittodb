cc=gcc
WFLAG=-Wall -Wextra -Wno-unused-parameter -Wno-unused-local-typedefs -Wimplicit-fallthrough
Headers=*.h
.PHONY: run debug parser valgrind

sql:main.c ${Headers}
	${cc} -ggdb ${WFLAG} -o sql *.c -I./ -lreadline 
run:
	./sql


debug:
	gf2 ./sql

valgrind:
	valgrind --leak-check=full --show-leak-kinds=all ./sql
