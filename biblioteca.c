#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
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

void sendFile( char * pathFile, int socket ){

    FILE * file = fopen(pathFile, "r");

    //Obtem tamanho do arquivo
    fseek(file, 0L, SEEK_END);
    long fileSize = ftell(file);

    //Calcula blocos
    int nBlocks = ceil((float) fileSize / FILE_BLOCK_SIZE);
    sendInt(nBlocks, socket);

    //Volta para o inicio do arquivo
    fseek(file, 0L, SEEK_SET);

    int blockSize = nBlocks == 1 ? fileSize : FILE_BLOCK_SIZE;
    void * bufferBlock = (void *) calloc(blockSize, 1);

    int nTotalBytesSendFile = 0;

    while (fread(bufferBlock, blockSize, 1, file)){

        sendInt(blockSize, socket);

        int nBytesToSendBlock = blockSize;
        int nTotalBytesSendBlock = 0;
        int nBytesSendBlock = 0;
        
        while (nBytesToSendBlock != 0){
            nBytesSendBlock = send(socket, bufferBlock + nTotalBytesSendBlock, nBytesToSendBlock, 0);
            nTotalBytesSendBlock += nBytesSendBlock;
            nBytesToSendBlock -= nBytesSendBlock;
        }

        nTotalBytesSendFile += nTotalBytesSendBlock;

        //Prepara ultimo bloco
        if ((--nBlocks) == 1){
            blockSize = fileSize - nTotalBytesSendFile;
            bufferBlock = (void *) realloc(bufferBlock, blockSize);
        }
    }

    fclose(file);    
}

void recvFile( char * pathNewFile, int socket){
    
    FILE * file = fopen(pathNewFile, "w");
    int nBlocks = recvInt(socket);

    for (int i = 0; i < nBlocks; i++) {
        int sizeBlock = recvInt(socket);
        void * block = (void *) calloc(sizeBlock, 1);

        recv(socket, block, sizeBlock, 0);
        fwrite(block, 1, sizeBlock, file);
    }

    fclose(file);
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
    char* value = (char*) calloc(nBytes + 1, sizeof(char));
    recv(socket, value, nBytes, 0);
    return value;
}

char * getMyLocalIP(){
    char hostName[256];
  
    gethostname(hostName, sizeof(hostName));
    struct hostent *host_entry = gethostbyname(hostName);
  
    char *ip = inet_ntoa(*((struct in_addr*)
                           host_entry->h_addr_list[0]));
    return ip;
}

