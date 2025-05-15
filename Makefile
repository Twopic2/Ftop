CC = gcc
CFLAGS = -Wall
DEGUG = -g

ftop: ftop.c
	$(CC) -o ftop $(CFLAGS) ftop.c disk.c cpuinfo.c -lncurses

clean:
	rm -rf ftop

run:
	./ftop

GDB: 
	$(CC) -o ftop $(CFLAGS) $(DEGUG) ftop.c disk.c cpuinfo.c -lncurses

start:
	gdb ./ftop