CC=gcc
CFLAGS=-Wall -g

all: tst shell

tst: tst.o readcmd.o

shell: shell.o readcmd.o -lpthread

clean:
	rm -f shell shell.o readcmd.o tst tst.o
