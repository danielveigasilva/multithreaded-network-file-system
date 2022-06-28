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

int initServer(int port, struct sockaddr_in * serv){
    int mysocket;                               /* socket used to listen for incoming connections */
    memset(serv, 0, sizeof(*serv));             /* zero the struct before filling the fields */
    serv->sin_family = AF_INET;                 /* set the type of connection to TCP/IP */
    serv->sin_addr.s_addr = htonl(INADDR_ANY);  /* set our address to any interface */
    serv->sin_port = htons(port);               /* set the server port number */

    mysocket = socket(AF_INET, SOCK_STREAM, 0);

    /* bind serv information to mysocket */
    bind(mysocket, (struct sockaddr *)serv, sizeof(struct sockaddr));

    /* start listening, allowing a queue of up to 1 pending connection */
    listen(mysocket, 1);

    return mysocket;
}

FileNameList * recvClientFileNames(int socket){
    
    FileNameList * list = initFileNameList();
    int countFiles = recvInt(socket);
    
    for (int i = 0; i < countFiles; i++){
        char * name = recvString(socket);
        addFileNameIntoFileNameList(list, name);
        
        //TODO: Remover log
        printf("* %s\n", name);
    }

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

void saveFileNames(ClientList * clientList, int socket){
    int idClient = recvInt(socket);
    FileNameList * fileNameList = recvClientFileNames(socket);
    addClientIntoClientList(clientList, idClient, fileNameList);
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
    int serverSocket = initServer(atoi(argv[1]), &serv);

    ClientList * clientList = initClientList();

    printf("Servidor ouvindo na porta: %s\n", argv[ 1 ] );

    while(true){
        printf("teste 1\n");
        int clientSocket = accept(serverSocket, (struct sockaddr *)&dest, &socksize);
        printf("teste 2\n");
        int command = recvInt(clientSocket);
        printf("teste 3\n");
        switch (command)
        {
            case LIST_COMMAND:
                //sendFilesNames(clientList, clientSocket);
                printf("LIST_COMMAND\n");
                break;

            case STATUS_COMMAND:
                printf("STATUS_COMMAND\n");
                break;
            
            case SEND_FILENAMES_COMMAND:
                saveFileNames(clientList, clientSocket);
                break;

            default:
                printf("ATENCAO: Comando inválido\n");
                break;
        }
    }

    close(serverSocket);
    return EXIT_SUCCESS;
}
