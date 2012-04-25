#CC=gcc -g -Wall -Werror
CC=clang -g -I/usr/include/x86_64-linux-gnu/ -Wall -Werror

OBJS=ilist.o socket.o http.o websocket.o messages.o processes.o
LIBS=-lev -lssl -ljson

tnrld: tnrld.c $(OBJS)
	$(CC) tnrld.c $(OBJS) -o tnrld $(LIBS)

%.o: %.c
	$(CC) -c $<	-o	$@

clean: 
	rm -f tnrld $(OBJS)
