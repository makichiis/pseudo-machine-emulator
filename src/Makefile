CC = gcc
CFLAG = -Wall -g

all: emulator

pme.o: pme.c
	$(CC) $(CFLAG) -c pme.c

emulator: pme.o
	$(CC) $(CFLAG) pme.o -o emulator

clean:
	rm -f *.o emulator


