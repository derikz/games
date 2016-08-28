
# Compiles on Ubuntu, openSUSE and FreeBSD without modification
# needs ncurses libraries

CC=cc 
COPTS=-Wall -pedantic

mathematico: mathematico.o instructions.o
	$(CC) $(COPTS) -omathematico mathematico.o instructions.o -lncurses

mathematico.o: mathematico.c
	$(CC) $(COPTS) -c mathematico.c

instructions.o: instructions.c
	$(CC) $(COPTS) -c instructions.c

clean:
	-rm *.o mathematico *~ pretty-print.pdf lint.out 2> /dev/null

lint: *.c
	splint *.c || true

print: *.c
	a2ps -R -g -o - *.c | ps2pdf - pretty-print.pdf
