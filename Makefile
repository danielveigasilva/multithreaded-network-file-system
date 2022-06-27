all: client server

client: client.o
	gcc -o client client.o biblioteca.o

client.o: client.c
	gcc -c -g client.c biblioteca.c

server: server.o
	gcc -o server server.o biblioteca.o

server.o: server.c
	gcc -c -g server.c biblioteca.c

clean: 
	rm *.o

