CC=gcc
CFLAGS=-Wall -Werror
LIBS=-lpthread
all: tst shell

tst: tst.o readcmd.o
	$(CC) -o tst tst.o readcmd.o

shell: shell.o readcmd.o csapp.o
		$(CC) -o shell shell.o readcmd.o csapp.o $(LIBS)

shell.o: shell.c readcmd.c csapp.c
		$(CC) $(CFLAGS) -c shell.c readcmd.c csapp.c $(LIBS)

readcmd.o: readcmd.c
		$(CC) $(CFLAGS) -c readcmd.c

csapp.o: csapp.c
		$(CC) $(CFLAGS) -c csapp.c $(LIBS)

clean:
	rm -f shell shell.o readcmd.o tst tst.o
