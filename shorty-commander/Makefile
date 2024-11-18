
CFLAGS := -O2 -std=c99 -Wall

shorty-commander: shorty-commander.o
	$(CC) -o shorty-commander shorty-commander.o -lusb-1.0

default: shorty-commander

clean:
	rm shorty-commander shorty-commander.o
