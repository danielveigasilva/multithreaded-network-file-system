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

typedef struct ArgsProcessCommandsFromClient_t{
    int socket;
    int idClient;   
    char * ipClient;
}ArgsProcessCommandsFromClient;

ClientList * clientList;

ClientList * initClientList(){
    ClientList * list = calloc(1, sizeof(ClientList));
    
    list->firstClient = NULL;
    list->lastClient = NULL;
    list->nClients = 0;
    
    return list;
}

void addClientIntoClientList(ClientList * list, Client * client){
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

Client* getClientByIdClient(ClientList * list, int idClient){
    Client * client = list->firstClient;
    while (client != NULL){
        if (client->idClient == idClient)
            return client;   
        client = client->nextClient;
    }
    return NULL;
}

Client* getClientByFileName(ClientList * list, char * name){
    Client * client = list->firstClient;
    while (client != NULL){
        FileName * fileName = client->FileNameList->firstFileName;
        while (fileName != NULL){
            if (!strcmp(fileName->name, name))
                return client;
            fileName = fileName->nextFileName;
        }
        client = client->nextClient;
    }
    return NULL;
}

void removeFileName(FileNameList * list, char * name){
    
    FileName * fileName = list->firstFileName;
    FileName * fileNameAnt = NULL;

    while (fileName != NULL){
        if (!strcmp(fileName->name, name)){

            if (list->firstFileName == fileName)
                list->firstFileName = fileName->nextFileName;
            
            if (list->lastFileName == fileName){
                list->lastFileName = fileNameAnt;
                if (list->lastFileName != NULL)
                    list->lastFileName->nextFileName = fileName->nextFileName;
            }
            
            if (fileNameAnt != NULL)
                fileNameAnt->nextFileName = fileName->nextFileName;
            
            list->nFileNames--;
            
            break;
        }

        fileNameAnt = fileName;
        fileName = fileName->nextFileName;
    }
}

void removeClient(ClientList * list, int idClient){
    
    Client * client = list->firstClient;
    Client * clientAnt = NULL;

    while (client != NULL){
        if (client->idClient == idClient){

            if (list->firstClient == client)
                list->firstClient = client->nextClient;
            
            if (list->lastClient == client){
                list->lastClient = clientAnt;
                if (list->lastClient != NULL)
                    list->lastClient->nextClient = client->nextClient;
            }
            
            if (clientAnt != NULL)
                clientAnt->nextClient = client->nextClient;
            
            list->nClients--;
            
            break;
        }

        clientAnt = client;
        client = client->nextClient;
    }
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

void saveClientInfo(ClientList * clientList, int idClient, char * ipClient, int socket){
    
    Client * client = calloc(1, sizeof(Client));
    client->idClient = idClient;
    client->nextClient = NULL;

    client->port = recvInt(socket);
    client->ip = (char*) calloc(sizeof(ipClient), sizeof(char));
    strcpy(client->ip, ipClient);
    client->FileNameList = recvClientFileNames(socket);

    addClientIntoClientList(clientList, client);

    //Envia id gerado
    sendInt(idClient, socket);

    printf(">> Cliente %d (%s : %d) - %d arquivos mapeados.\n", client->idClient, client->ip, client->port, client->FileNameList->nFileNames);
}

void sendClient(Client * client, int socket){
    sendString(client->ip, socket);
    sendInt(client->port, socket);
    sendInt(client->idClient, socket);
}

void sendClientOfId(ClientList * clientList, int socket){
    int idClient = recvInt(socket);
    Client * client = getClientByIdClient(clientList, idClient);

    //Status
    sendInt(client == NULL ? STATUS_NOT_FOUND : STATUS_OK, socket);
    
    if (client != NULL)
        sendClient(client, socket);
}

void sendClientOfFile(ClientList * clientList, int socket){
    char * fileName = recvString(socket);
    Client * client = getClientByFileName(clientList, fileName);

    //Status
    sendInt(client == NULL ? STATUS_NOT_FOUND : STATUS_OK, socket);
    
    if (client != NULL)
        sendClient(client, socket);
}

void deleteFileNameFromClient(ClientList * clientList, int socket){
    
    char * fileName = recvString(socket);
    Client * client = getClientByFileName(clientList, fileName);
    
    removeFileName(client->FileNameList, fileName);
}

void addFileNameFromClient(ClientList * clientList , int socket){
    
    int idClient = recvInt(socket);
    char * fileName = recvString(socket);
    Client * client = getClientByIdClient(clientList, idClient);

    addFileNameIntoFileNameList(client->FileNameList, fileName);
}

void deleteClient(ClientList * clientList , int socket){
    int idClient = recvInt(socket);
    removeClient(clientList, idClient);
    close(socket);
    printf(">> Cliente %d - Desconectado.\n", idClient);
}

void sendStatus(ClientList * clientList, int socket){
    
    Client * clientAux = clientList->firstClient;

    sendInt(clientList->nClients, socket);
    
    while(clientAux != NULL){
    
        sendClient(clientAux, socket);

        clientAux = clientAux->nextClient;
    }   
}

void * processCommandsFromClient(void * arg){

    ArgsProcessCommandsFromClient *argumentos = (ArgsProcessCommandsFromClient*)arg;
    int clientSocket = argumentos->socket;

    while (true){
        int command = recvInt(clientSocket);
        switch (command)
        {
            case LIST_COMMAND:
                sendFilesNames(clientList, clientSocket);
                break;

            case GET_STATUS_COMMAND:
                sendStatus(clientList, clientSocket);
                break;
            
            case SEND_CLIENT_INFO_COMMAND:
                saveClientInfo(clientList, argumentos->idClient, argumentos->ipClient, clientSocket);
                break;
            
            case GET_CLIENT_BY_FILE_COMMAND:
                sendClientOfFile(clientList, clientSocket);
                break;

            case GET_CLIENT_BY_ID_COMMAND:
                sendClientOfId(clientList, clientSocket);
                break;

            case DELETE_FILE_CLIENT_COMMAND:
                deleteFileNameFromClient(clientList, clientSocket);
                break;
            
            case ADD_FILE_CLIENT_COMMAND:
                addFileNameFromClient(clientList, clientSocket);
                break;
            
            case DELETE_CLIENT_COMMAND:
                deleteClient(clientList, clientSocket);
                return (void*) 0; //Mata a thread de conexão com o client deletado
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

    printf("Servidor ouvindo em %s : %s\n", inet_ntoa(serv.sin_addr), argv[1] );

    pthread_t * threadClient = NULL;
    int countClients = 0;

    while(true){
        int clientSocket = accept(serverSocket, (struct sockaddr *)&dest, &socksize);
        char * ipClient = inet_ntoa(dest.sin_addr);

        threadClient = (pthread_t*) calloc(1, sizeof(pthread_t));

        ArgsProcessCommandsFromClient * args = (ArgsProcessCommandsFromClient *) calloc(1, sizeof(ArgsProcessCommandsFromClient));
        args->socket = clientSocket;
        args->idClient = countClients;
        args->ipClient = (char*) calloc(sizeof(ipClient), sizeof(char));
        strcpy(args->ipClient, ipClient);

        pthread_create(threadClient, NULL, processCommandsFromClient, args);
        countClients++;
    }

    close(serverSocket);
    return EXIT_SUCCESS;
}
