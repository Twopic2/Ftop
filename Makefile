CC = gcc
CFLAGS = -Wall -Wextra
DEGUG_FLAGS = -g3 -O0
LIBS = -lncurses

SOURCES = ftop.c memalloc.c disk.c cpuinfo.c chart.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = ftop
HEADERS = memalloc.h disk.h cpuinfo.h chart.h

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LIBS)

ftop.o: ftop.c $(HEADERS)
	$(CC) $(CFLAGS) -c ftop.c

memalloc.o: memalloc.c memalloc.h
	$(CC) $(CFLAGS) -c memalloc.c

disk.o: disk.c disk.h
	$(CC) $(CFLAGS) -c disk.c

cpuinfo.o: cpuinfo.c cpuinfo.h
	$(CC) $(CFLAGS) -c cpuinfo.c

chart.o: chart.c chart.h
	$(CC) $(CFLAGS) -c chart.c

clean:
	rm -rf $(OBJECTS) $(TARGET) *.dSYM

run: $(TARGET)
	./$(TARGET)
	
debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean $(TARGET)

release: CFLAGS += $(RELEASE_FLAGS)
release: clean $(TARGET)


valgrind: debug
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--verbose \
		--log-file=valgrind-report.txt \
		./$(TARGET)

gdb: debug
	cgdb ./$(TARGET)
