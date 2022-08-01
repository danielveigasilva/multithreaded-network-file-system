all: clientMac serverMac

mac: clientMac serverMac

linux: clientLinux serverLinux

clientLinux: client.o
	gcc -o client client.o biblioteca.o -lm -lpthread

clientMac: client.o
	gcc -o client client.o biblioteca.o

client.o: client.c
	gcc -c -g client.c biblioteca.c

serverMac: server.o
	gcc -o server server.o biblioteca.o

serverLinux: server.o
	gcc -o server server.o biblioteca.o -lm -lpthread

server.o: server.c
	gcc -c -g server.c biblioteca.c

clean: 
	rm *.o

