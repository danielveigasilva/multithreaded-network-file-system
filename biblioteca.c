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

    //Calcula tamanho do ultimo bloco
    int lastBlockSize = nBlocks == 1 ? fileSize : FILE_BLOCK_SIZE - ((nBlocks * FILE_BLOCK_SIZE) - fileSize);
    sendInt(lastBlockSize, socket);

    //Volta para o inicio do arquivo
    fseek(file, 0L, SEEK_SET);

    int blockSize = nBlocks == 1 ? lastBlockSize : FILE_BLOCK_SIZE;
    void * bufferBlock = (void *) calloc(blockSize, 1);

    //printf("LOG: nBlock = %d | lastBlock = %d | sizeFile = %ld\n", nBlocks, lastBlockSize, fileSize);

    while (nBlocks > 0){

        fread(bufferBlock, blockSize, 1, file);

        int nBytesToSendBlock = blockSize;
        int nTotalBytesSendBlock = 0;
        int nBytesSendBlock = 0;
        
        while (nBytesToSendBlock != 0){
            nBytesSendBlock = send(socket, bufferBlock + nTotalBytesSendBlock, nBytesToSendBlock, 0);
            nTotalBytesSendBlock += nBytesSendBlock;
            nBytesToSendBlock -= nBytesSendBlock;

            //if (nBytesToSendBlock != 0)
            //    printf("LOG: Reenviando .... enviei bloco %d | size %d\n", nBlocks, blockSize);
        }

        //Prepara ultimo bloco
        if ((--nBlocks) == 1){
            blockSize = lastBlockSize;

            free(bufferBlock);
            bufferBlock = (void *) calloc(blockSize, 1);
        }
    }

    fclose(file);    
}

void recvFile( char * pathNewFile, int socket ){

    FILE * file = fopen(pathNewFile, "w");
    int nBlocks = recvInt(socket);
    int lastBlockSize = recvInt(socket);

    for (int i = 0; i < nBlocks; i++) {

        int blockSize = i == (nBlocks - 1) ? lastBlockSize : FILE_BLOCK_SIZE;
        void * block = (void *) calloc(blockSize, 1);
        
        int nBytesToRecvBlock = blockSize;
        int nTotalBytesRecvBlock = 0;
        int nBytesRecvBlock = 0;
        
        while (nBytesToRecvBlock != 0){
            nBytesRecvBlock = recv(socket, block + nTotalBytesRecvBlock, nBytesToRecvBlock, 0);
            nTotalBytesRecvBlock += nBytesRecvBlock;
            nBytesToRecvBlock -= nBytesRecvBlock;
        }

        //recv(socket, block, blockSize, 0);
        fwrite(block, 1, blockSize, file);
        free(block);

        //printf("LOG: block[%d].size = %d\n", i, blockSize);
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

