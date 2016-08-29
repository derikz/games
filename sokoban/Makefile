
# Compiles on FreeBSD without modification
# needs Free Pascal and its libraries

sokoban: sokoban.pas
	fpc sokoban.pas

clean:
	-rm *.o pretty-print.pdf sokoban 2> /dev/null

print: *.c
	a2ps -R -g -o - *.pas | ps2pdf - pretty-print.pdf
