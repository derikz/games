
# Compiles on Ubuntu, openSUSE and FreeBSD without modification
# needs ncurses libraries

COPTS=-Wall -pedantic
CC=cc

hectic: hectic.o instructions.o
	$(CC) $(COPTS) -o hectic hectic.o instructions.o -lncurses

hectic.o: hectic.c
	$(CC) $(COPTS) -c hectic.c

instructions.o: instructions.c
	$(CC) $(COPTS) -c instructions.c

clean:
	-rm *.o hectic *~ pretty-print.pdf lint.out 2> /dev/null

lint: *.c
	splint *.c || true

print: *.c
	a2ps -R -g -o - *.c | ps2pdf - pretty-print.pdf
