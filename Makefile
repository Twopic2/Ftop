

ftop: ftop.c
	gcc -o ftop ftop.c disk.c -lncurses

clean:
	rm -rf ftop
