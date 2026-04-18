CC = gcc
CFLAGS = -Wall -I.
LDFLAGS = -lpthread

all: tiny cgi-bin/adder

tiny: tiny.c csapp.c
	$(CC) $(CFLAGS) -o tiny tiny.c csapp.c $(LDFLAGS)

cgi-bin/adder: cgi-bin/adder.c csapp.c
	$(CC) $(CFLAGS) -o cgi-bin/adder cgi-bin/adder.c csapp.c $(LDFLAGS)

clean:
	rm -f tiny cgi-bin/adder

.PHONY: all clean