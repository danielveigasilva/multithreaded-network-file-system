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
#include <pthread.h>
#include "biblioteca.h"

#define MAXRCVLEN 500

typedef struct ArgsInitClientServer_t{
    int portClient;
    int ipClient;   
    char * directory;
}ArgsInitClientServer;


typedef struct ArgsProcessCommandsFromOtherClient_t{
    int socket;
    char * directory;
}ArgsProcessCommandsFromOtherClient;


int mapCommand(char* command){
    if (!strcmp(command, "list"))
        return LIST_COMMAND;
    if (!strcmp(command, "status"))
        return STATUS_COMMAND;
    if (!strcmp(command, "sync"))
        return SEND_CLIENT_INFO_COMMAND;
    if (!strcmp(command, "info"))
        return CLIENT_INFO_COMMAND;
    if (!strcmp(command, "del"))
        return DELETE_FILE_CLIENT_COMMAND;
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

Client* recvClientInfo(int socket){
    Client * client = calloc(1, sizeof(Client));

    int status = recvInt(socket);
    if (status == 404)
        return NULL;

    client->ip = recvInt(socket);
    client->port = recvInt(socket);
    client->idClient = recvInt(socket);

    return client;
}

int conectSocketServer(int portServer, int ipServer){

    struct sockaddr_in dest;
    int socketServer = socket(AF_INET, SOCK_STREAM, 0);

    memset(&dest, 0, sizeof(dest));                
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = htonl(ipServer); 
    dest.sin_port = htons(portServer); 

    int connectResult = connect(socketServer, (struct sockaddr *)&dest, sizeof(struct sockaddr_in));
    if( connectResult == - 1 )
   	    return -1;

    return socketServer;
}




void deleteFile(char * directory, int socket){
    //Montando path file
    char * fileName = recvString(socket);
    char * pathFile = directory; 
    strcat(pathFile, fileName);

    remove(pathFile);
}

void * processCommandsFromOtherClient(void * args){
    ArgsProcessCommandsFromOtherClient * argumentos = (ArgsProcessCommandsFromOtherClient*) args; 
    
    int socket = argumentos->socket;
    char * directory = argumentos->directory;

    while (true){
        int command = recvInt(socket);
        switch (command){

            case DELETE_FILE_CLIENT_COMMAND:
                deleteFile(directory, socket);
                break;
        }
    }
    return (void*) 0;
}

void * initClientServer(void * args){

    ArgsInitClientServer *argumentos = (ArgsInitClientServer*)args;
    int portClient = argumentos->portClient;
    int ipClient = argumentos->ipClient;
    char * directory = argumentos->directory;


    struct sockaddr_in dest; 
    struct sockaddr_in serv;
    socklen_t socksize = sizeof(struct sockaddr_in);

    memset(&serv, 0, sizeof(serv));             
    serv.sin_family = AF_INET;                 
    serv.sin_addr.s_addr = htonl(ipClient);
    serv.sin_port = htons(portClient);   

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    bind(serverSocket, (struct sockaddr *)&serv, sizeof(struct sockaddr));
    listen(serverSocket, 1);

    pthread_t * threadClient = NULL;

    while(true){
        int clientSocket = accept(serverSocket, (struct sockaddr *)&dest, &socksize);
        
        ArgsProcessCommandsFromOtherClient * args = (ArgsProcessCommandsFromOtherClient *) calloc(1, sizeof(ArgsProcessCommandsFromOtherClient));
        args->directory = directory;
        args->socket = clientSocket;
        
        threadClient = (pthread_t*) calloc(1, sizeof(pthread_t));
        pthread_create(threadClient, NULL, processCommandsFromOtherClient, args);
    }

    close(serverSocket);
    return (void *) EXIT_SUCCESS;
}



void recvFilesNames(int socket){
    
    sendInt(LIST_COMMAND, socket);
    int countClients = recvInt(socket);
    
    for (int i = 0; i < countClients; i++){
        
        int idCliente = recvInt(socket);
        int countFileNames = recvInt(socket);

        printf("#Cliente - %d\n", idCliente);

        for (int j = 0; j < countFileNames; j++)
            printf("  * %s\n", recvString(socket));   
    }
}

void sendClientInfo(int socket, int portClient, int ipClient, char * directory){
    sendInt(SEND_CLIENT_INFO_COMMAND, socket);
    
    sendInt(portClient, socket);
    sendInt(ipClient, socket);
    sendClientFileNames(directory, socket);
}

void getClientInfo(int socket){
    
    sendInt(CLIENT_INFO_COMMAND, socket);

    int idClient = 0;
    scanf("%d", &idClient);
    sendInt(idClient, socket);

    Client * client = recvClientInfo(socket);

    if (client == NULL)
        printf("#ERRO: Cliente não localizado.\n");
    else{
        printf("#Cliente - %d\n", client->idClient);
        printf("  * IP: %d\n", client->ip);
        printf("  * PORTA: %d\n", client->port);
    }  
}

void deleteFileClient(int socket){
    char * fileName;
    scanf("%s", fileName);

    //Obtem dados do cliente com o arquivo
    sendInt(DELETE_FILE_CLIENT_COMMAND, socket);
    sendString(fileName, socket);
    Client * client = recvClientInfo(socket);

    if (client == NULL)
        printf("#ERRO: Arquivo não localizado.\n");
    else {
        //Conecta ao server do cliente
        int socketServerClient = conectSocketServer(client->port, client->ip);
        
        //Solicita delete arquivo
        sendInt(DELETE_FILE_CLIENT_COMMAND, socketServerClient);
        sendString(fileName, socketServerClient);
    }  
}



int main(int argc, char *argv[])
{
	if( argc < 4 ){ 
        printf("ATENCAO: quantidade de argumentos insuficiente.\n");
        return EXIT_FAILURE;
    }

    //Inicialização de variáveis
    char * directory    = argv[1];
    int portServer      = atoi(argv[2]);
    int portClient      = atoi(argv[3]);
    int ipServer        = INADDR_LOOPBACK; //TODO: Passar como parametro
    int ipClient        = INADDR_LOOPBACK;

    //Conexão com servidor
    int socketServer = conectSocketServer(portServer, ipServer);
    if (socketServer < 0){
        printf("FALHA: Erro ao se conectar ao servidor - %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    //Realizando sincronização de arquivos disponíveis
    sendClientInfo(socketServer, portClient, ipClient, directory);

    //Criação de thread para mini-servidor
    ArgsInitClientServer * args = (ArgsInitClientServer *) calloc(1, sizeof(ArgsInitClientServer));
    args->portClient = portClient;
    args->ipClient = ipClient;
    args->directory = directory;

    pthread_t * threadClient = (pthread_t*) calloc(1, sizeof(pthread_t));
    pthread_create(threadClient, NULL, initClientServer, args);
                
    //Laço de comandos
    while (true)
    {
        char command[500];
        printf(">");
        scanf("%s", command);

        int codCommand = mapCommand(command);

        switch (codCommand)
        {
            case LIST_COMMAND:
                recvFilesNames(socketServer);
                break;

            case SEND_CLIENT_INFO_COMMAND:
                sendClientInfo(socketServer, portClient, ipClient, directory);
                break;

            case CLIENT_INFO_COMMAND:
                getClientInfo(socketServer);
                break;

            case DELETE_FILE_CLIENT_COMMAND:
                deleteFileClient(socketServer);
                break;
        }
        
    }

    return EXIT_SUCCESS;
}
