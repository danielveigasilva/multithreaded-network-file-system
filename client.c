#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <dirent.h>
#include "biblioteca.h"

#define MAXRCVLEN 500


int mapCommand(char* command){
    if (!strcmp(command, "list"))
        return LIST_COMMAND;
    if (!strcmp(command, "status"))
        return STATUS_COMMAND;
    if (!strcmp(command, "sync"))
        return SEND_FILENAMES_COMMAND;
    return -1;
}

void sendClientFileNames(char * directory, int socket){
    
    DIR *dir = opendir(directory);
    struct dirent * entry = NULL;
    char ** nameFiles = NULL;
    int countFiles = 0;

    if (dir != NULL){
        while((entry = readdir(dir)) != NULL){
            //Se arquivo for especial ignora
            if (entry->d_name[0] == '.')
                continue;
            
            nameFiles = (char**) realloc(nameFiles, (countFiles + 1) * sizeof(char*));
            nameFiles[countFiles] = (char*) calloc((entry->d_namlen + 1), sizeof(char));
            strcpy(nameFiles[countFiles], entry->d_name);
            
            countFiles++;
        }

        sendInt(countFiles, socket);

        for (int i = 0; i < countFiles; i++)
            sendString(nameFiles[i], socket);
    }
} 

void recvFilesNames(int socket){
    int countClients = recvInt(socket);
    
    for (int i = 0; i < countClients; i++){
        
        int idCliente = recvInt(socket);
        int countFileNames = recvInt(socket);

        printf("#Cliente - %d\n", idCliente);

        for (int j = 0; j < countFileNames; j++)
            printf("  * %s\n", recvString(socket));   
    }
}

int main(int argc, char *argv[])
{
	if( argc < 3 ){ 
        printf("ATENCAO: quantidade de argumentos insuficiente.\n");
        return EXIT_FAILURE;
    }

    char buffer[MAXRCVLEN + 1]; /* +1 so we can add null terminator */
    bzero( buffer, MAXRCVLEN + 1 );

    struct sockaddr_in dest;
    char * directory = argv[1];


    int mysocket = socket(AF_INET, SOCK_STREAM, 0);

    memset(&dest, 0, sizeof(dest));                
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = htonl(INADDR_LOOPBACK); 
    dest.sin_port = htons(atoi(argv[2])); 

    int connectResult = connect(mysocket, (struct sockaddr *)&dest, sizeof(struct sockaddr_in));
    if( connectResult == - 1 ){
   	    printf("FALHA: Erro ao se conectar ao servidor - %s\n", strerror(errno));
   		return EXIT_FAILURE;
    }

    //Realizando sincronização de arquivos disponíveis
    sendInt(SEND_FILENAMES_COMMAND, mysocket);
    sendClientFileNames(directory, mysocket);
    
    while (true)
    {
        char command[500];
        printf(">");
        scanf("%s", command);

        int codCommand = mapCommand(command);
        sendInt(codCommand, mysocket);

        switch (codCommand)
        {
            case LIST_COMMAND:
                recvFilesNames(mysocket);
                break;
            case SEND_FILENAMES_COMMAND:
                sendInt(SEND_FILENAMES_COMMAND, mysocket);
                sendClientFileNames(directory, mysocket);
                break;
        }
        
    }

    //close(socket);
    //connect(socket, (struct sockaddr *)&dest, sizeof(struct sockaddr_in));

    return EXIT_SUCCESS;
}
