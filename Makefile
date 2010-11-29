# Compilador x86
CC=gcc

CFLAGS=-c -Wall

all: formsdetector

formsdetector: main.o framegrabber.o formsdetector.o
	$(CC) main.o framegrabber.o formsdetector.o -o formsdetector

main.o: main.c
	$(CC) $(CFLAGS) main.c

formsdetector.o: formsdetector.c
	$(CC) $(CFLAGS) formsdetector.c

framegrabber.o: framegrabber.c
	$(CC) $(CFLAGS) framegrabber.c

clean:
	rm -rf *o formsdetector

