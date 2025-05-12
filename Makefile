
ftop: ftop.c
	gcc -g -o ftop ftop.c disk.c cpuinfo.c -lncurses

clean:
	rm -rf ftop

run:
	./ftop