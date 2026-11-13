cc=gcc
WFLAG=-Wall -Wextra -Wno-unused-parameter -Wno-unused-local-typedefs -Wimplicit-fallthrough 
Headers=*.h
.PHONY: run debug valgrind sql main

sql:main.c ${Headers}
	${cc} -ggdb -fsanitize=address,leak ${WFLAG} -o sql *.c -I./ -lreadline 

main:
	bear -- make sql

run:
	./sql


debug:
	gf2 ./sql

valgrind:
	valgrind --leak-check=full  ./sql
