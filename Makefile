CC=gcc
CFLAGS=-Wall -g
LIBS=-lpthread

ACS: acs.c linkedls.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f ACS *.o