all: setitimer-helper squish-unix

CC = gcc
CFLAGS = -Wall -W
LOADLIBES = -lm
setitimer-helper: setitimer-helper.o
squish-unix: squish-unix.o

clean: 
	rm -f *.o setitimer-helper squish-unix
