#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <pthread.h>
#include "biblioteca.h"

typedef struct FileName_t{
    char * name;
    struct FileName_t * nextFileName;
}FileName;

typedef struct FileNameList_t{
    int nFileNames;
    FileName * firstFileName;
    FileName * lastFileName;
}FileNameList;

typedef struct Client_t{
    unsigned int idClient;
    FileNameList * FileNameList;
    struct Client_t * nextClient;
}Client;

typedef struct ClientList_t{
    int nClients;
    Client * firstClient;
    Client * lastClient;
}ClientList;

typedef struct ArgsProcessCommandsFromClient_t{
    int socket;
    int idClient;   
}ArgsProcessCommandsFromClient;

ClientList * clientList;

ClientList * initClientList(){
    ClientList * list = calloc(1, sizeof(ClientList));
    
    list->firstClient = NULL;
    list->lastClient = NULL;
    list->nClients = 0;
    
    return list;
}

void addClientIntoClientList(ClientList * list, int idClient, FileNameList * fileNameList){
    Client * client = calloc(1, sizeof(Client));
    client->idClient = idClient;
    client->FileNameList = fileNameList;
    client->nextClient = NULL;

    if (list->firstClient == NULL && list->lastClient == NULL){
        list->firstClient = client;
        list->lastClient = client;
    }
    else{
        list->lastClient->nextClient = client;
        list->lastClient = client; 
    }

    list->nClients ++;
}

FileNameList * initFileNameList(){
    FileNameList * list = calloc(1, sizeof(FileNameList));
    
    list->firstFileName = NULL;
    list->lastFileName = NULL;
    list->nFileNames = 0;
    
    return list;
}

void addFileNameIntoFileNameList(FileNameList * list, char * name){
    FileName * fileName = calloc(1, sizeof(FileName));
    fileName->name = name;
    fileName->nextFileName = NULL;

    if (list->firstFileName == NULL && list->lastFileName == NULL){
        list->firstFileName = fileName;
        list->lastFileName = fileName;
    }
    else{
        list->lastFileName->nextFileName = fileName;
        list->lastFileName = fileName; 
    }

    list->nFileNames ++;
}

FileNameList * recvClientFileNames(int socket){
    
    FileNameList * list = initFileNameList();
    int countFiles = recvInt(socket);
    
    for (int i = 0; i < countFiles; i++)
        addFileNameIntoFileNameList(list, recvString(socket));

    return list;   
}

void sendFilesNames(ClientList * clientList, int socket){

    //Quantidade de clientes
    sendInt(clientList->nClients, socket);

    Client * client = clientList->firstClient;
    while (client != NULL){
        //ID Cliente
        sendInt(client->idClient, socket);
        //Quantidade de arquivos
        sendInt(client->FileNameList->nFileNames, socket);
        
        FileName * fileName = client->FileNameList->firstFileName;
        while (fileName != NULL){
            //Nome Arquivo
            sendString(fileName->name, socket);
            fileName = fileName->nextFileName;
        }
        client = client->nextClient;
    }
    
}

void saveFileNames(ClientList * clientList, int idClient, int socket){
    FileNameList * fileNameList = recvClientFileNames(socket);
    addClientIntoClientList(clientList, idClient, fileNameList);

    printf(">> %d arquivos mapeados no cliente %d.\n", fileNameList->nFileNames, idClient);
}

void * processCommandsFromClient(void * arg){

    ArgsProcessCommandsFromClient *argumentos = (ArgsProcessCommandsFromClient*)arg;
    int clientSocket = argumentos->socket;
    int idClient = argumentos->idClient;

    while (true){
        int command = recvInt(clientSocket);
        switch (command)
        {
            case LIST_COMMAND:
                sendFilesNames(clientList, clientSocket);
                break;

            case STATUS_COMMAND:
                printf("STATUS_COMMAND\n");
                break;
            
            case SEND_FILENAMES_COMMAND:
                saveFileNames(clientList, idClient, clientSocket);
                break;

            default:
                printf("ATENCAO: Comando inválido\n");
                break;
        }
    }
    return (void*) 0;
}

int main(int argc, char *argv[])
{
    if( argc != 2 ){ 
        printf("ATENCAO: informe a porta do servidor\n");
        return EXIT_FAILURE;
    }

    struct sockaddr_in dest; 
    struct sockaddr_in serv;
    socklen_t socksize = sizeof(struct sockaddr_in);

    memset(&serv, 0, sizeof(serv));             
    serv.sin_family = AF_INET;                 
    serv.sin_addr.s_addr = htonl(INADDR_ANY);
    serv.sin_port = htons(atoi(argv[1]));   

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    bind(serverSocket, (struct sockaddr *)&serv, sizeof(struct sockaddr));
    listen(serverSocket, 1);

    clientList = initClientList();

    printf("Servidor ouvindo na porta: %s\n", argv[ 1 ] );

    pthread_t * threadClient = NULL;
    int countClients = 0;

    while(true){
        int clientSocket = accept(serverSocket, (struct sockaddr *)&dest, &socksize);
        
        threadClient = (pthread_t*) calloc(1, sizeof(pthread_t));
        
        ArgsProcessCommandsFromClient * args = (ArgsProcessCommandsFromClient *) calloc(1, sizeof(ArgsProcessCommandsFromClient));
        args->socket = clientSocket;
        args->idClient = countClients;

        pthread_create(threadClient, NULL, processCommandsFromClient, args);
        countClients++;
    }

    close(serverSocket);
    return EXIT_SUCCESS;
}
