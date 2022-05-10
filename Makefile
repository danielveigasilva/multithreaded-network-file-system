all: client server

client: client.o
	gcc -o client client.o

client.o: client.c
	gcc -c -g client.c

server: server.o
	gcc -o server server.o

server.o: server.c
	gcc -c -g server.c

clean: 
	rm *.o

