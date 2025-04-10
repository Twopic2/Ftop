

ftop: ftop.c
	gcc -o ftop ftop.c -lncurses

clean:
	rm -rf ftop
