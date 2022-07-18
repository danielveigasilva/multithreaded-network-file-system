#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include "biblioteca.h"

void sendInt( int value, int socket ){
    send(socket, &value, sizeof(int), 0);    
}

void sendDouble( double value, int socket ){
    send(socket,&value, sizeof(double), 0);
}

void sendString( char* value, int socket ){
    int nBytes = strlen(value) * sizeof(char);
    sendInt(nBytes, socket);
    send(socket, value, nBytes, 0);    
}

int recvInt( int socket ){
    int value;
    recv(socket, &value, sizeof(int), 0);
    return value;
}

double recvDouble( int socket ){
    double value;
    recv(socket, &value, sizeof(double), 0);
    return value;
}

char* recvString( int socket ){
    int nBytes = recvInt(socket);
    char* value = (char*) calloc(sizeof(char), nBytes + 1);
    recv(socket, value, nBytes, 0);
    return value;
}

