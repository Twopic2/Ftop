
ftop: ftop.c
	gcc -g -o  ftop ftop.c disk.c -lncurses

clean:
	rm -rf ftop
