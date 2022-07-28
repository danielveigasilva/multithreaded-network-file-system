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
#include <fcntl.h>
#include <time.h>
#include "biblioteca.h"

#define MAXRCVLEN 500

typedef struct ArgsInitClientServer_t{
    struct sockaddr_in clientServ;  
    int socketServer;
    char * directory;
}ArgsInitClientServer;


typedef struct ArgsProcessCommandsFromOtherClient_t{
    int socketClient;
    int socketServer;
    char * directory;
}ArgsProcessCommandsFromOtherClient;


typedef struct ArgsGetFileClientAndSave_t{
    int socket;
    int idClient;
    char * fileName;
    char * directory;
    Client * client;
}ArgsGetFileClientAndSave;



int mapCommand(char* command){
    if (!strcmp(command, "list"))
        return LIST_COMMAND;
    if (!strcmp(command, "status"))
        return STATUS_COMMAND;
    //if (!strcmp(command, "sync"))
    //    return SEND_CLIENT_INFO_COMMAND;
    if (!strcmp(command, "info"))
        return CLIENT_INFO_COMMAND;
    if (!strcmp(command, "rm"))
        return DELETE_FILE_CLIENT_COMMAND;
    if (!strcmp(command, "get"))
        return GET_FILE_COMMAND;
    return -1;
}

int conectSocketServer(int portServer, char * ipServer){
    //TODO: Ajuster parametro ipServer
    struct sockaddr_in dest;
    int socketServer = socket(AF_INET, SOCK_STREAM, 0);

    memset(&dest, 0, sizeof(dest));                
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = inet_addr(ipServer); 
    dest.sin_port = htons(portServer); 

    int connectResult = connect(socketServer, (struct sockaddr *)&dest, sizeof(struct sockaddr_in));
    if( connectResult == - 1 )
   	    return -1;

    return socketServer;
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

    client->ip = recvString(socket);
    client->port = recvInt(socket);
    client->idClient = recvInt(socket);

    return client;
}

void deleteFile(char * directory, int socketClient, int socketServer){
    //Montando path file
    char * fileName = recvString(socketClient);
    char pathFile[strlen(directory) + strlen(fileName) + 1];
    strcpy(pathFile, directory); 
    strcat(pathFile, fileName);

    if (!remove(pathFile)){
        //Atualiza estrutura do server
        sendInt(DELETE_FILE_CLIENT_COMMAND, socketServer);
        sendString(fileName, socketServer);
    }
    else    
        printf("ERRO: %s\n", pathFile);
}

void sendFileClient(char * directory, int socketClient){
    //Montando path file
    char * fileName = recvString(socketClient);
    char pathFile[strlen(directory) + strlen(fileName) + 1];
    strcpy(pathFile, directory); 
    strcat(pathFile, fileName);

    //Envia arquivo
    sendFile(pathFile, socketClient);
}

void * processCommandsFromOtherClient(void * args){
    ArgsProcessCommandsFromOtherClient * argumentos = (ArgsProcessCommandsFromOtherClient*) args; 
    
    int socketClient = argumentos->socketClient;
    int socketServer = argumentos->socketServer;
    char * directory = argumentos->directory;

    while (true){
        int command = recvInt(socketClient);
        switch (command){

            case DELETE_FILE_CLIENT_COMMAND:
                deleteFile(directory, socketClient, socketServer);
                break;
            
            case GET_FILE_COMMAND:
                sendFileClient(directory, socketClient);
                break;
        }
    }
    return (void*) 0;
}

void * initClientServer(void * args){

    ArgsInitClientServer *argumentos = (ArgsInitClientServer*)args;
    int socketServer = argumentos->socketServer;
    char * directory = argumentos->directory;
    struct sockaddr_in serv = argumentos->clientServ;

    struct sockaddr_in dest;
    socklen_t socksize = sizeof(struct sockaddr_in);   

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    bind(serverSocket, (struct sockaddr *)&serv, sizeof(struct sockaddr));
    listen(serverSocket, 1);

    pthread_t * threadClient = NULL;

    while(true){
        int clientSocket = accept(serverSocket, (struct sockaddr *)&dest, &socksize);
        
        ArgsProcessCommandsFromOtherClient * args = (ArgsProcessCommandsFromOtherClient *) calloc(1, sizeof(ArgsProcessCommandsFromOtherClient));
        args->directory = directory;
        args->socketClient = clientSocket;
        args->socketServer = socketServer;
        
        threadClient = (pthread_t*) calloc(1, sizeof(pthread_t));
        pthread_create(threadClient, NULL, processCommandsFromOtherClient, args);
    }

    close(serverSocket);
    return (void *) EXIT_SUCCESS;
}



void recvFilesNames(int socket){
    
    sendInt(LIST_COMMAND, socket);
    int countClients = recvInt(socket);

    printf("\n");
    for (int i = 0; i < countClients; i++){
        
        int idCliente = recvInt(socket);
        int countFileNames = recvInt(socket);

        printf(" Cliente - %d\n", idCliente);

        for (int j = 0; j < countFileNames; j++)
            printf("  * %s\n", recvString(socket));
        printf("\n");
    }
}

int sendClientInfo(int socket, int portClient, char * ipClient, char * directory){
    sendInt(SEND_CLIENT_INFO_COMMAND, socket);
    
    sendInt(portClient, socket);
    sendString(ipClient, socket);
    sendClientFileNames(directory, socket);

    int idClient = recvInt(socket);
    return idClient;
}

void getClientInfo(int socket){
    
    sendInt(CLIENT_INFO_COMMAND, socket);

    int idClient = 0;
    scanf("%d", &idClient);
    sendInt(idClient, socket);

    Client * client = recvClientInfo(socket);

    if (client == NULL)
        printf(" FALHA: Cliente não localizado.\n");
    else{
        printf(" Cliente - %d\n", client->idClient);
        printf("  * IP: %s\n", client->ip);
        printf("  * PORTA: %d\n", client->port);
    }  
}

void deleteFileClient(int socket){
    char fileName [MAXRCVLEN];
    scanf("%s", fileName);

    //Obtem dados do cliente com o arquivo
    sendInt(GET_CLIENT_CONECT_COMMAND, socket);
    sendString(fileName, socket);
    Client * client = recvClientInfo(socket);

    if (client == NULL)
        printf(" FALHA: Arquivo não localizado.\n");
    else {
        //Conecta ao server do cliente
        int socketServerClient = conectSocketServer(client->port, client->ip);
        
        //Solicita delete arquivo
        sendInt(DELETE_FILE_CLIENT_COMMAND, socketServerClient);
        sendString(fileName, socketServerClient);

        close(socketServerClient);
    }  
}

void * getFileClientAndSave(void * _args){

    ArgsGetFileClientAndSave * args = (ArgsGetFileClientAndSave *) _args;

    int socketServer = args->socket;
    int myId = args->idClient;
    char * fileName = args->fileName; 
    char * directory = args->directory;
    Client * client = args->client;

    if (client == NULL){
        printf("Arquivo %s nao localizado.\n", fileName);
        return (void *) EXIT_FAILURE;
    }
    else if (myId == client->idClient){ //TODO: Verificar se trava é válida
        printf("Arquivo %s já pertence ao cliente.\n", fileName);
        return (void *) EXIT_FAILURE;
    } 
    else {
        //Conecta ao server do cliente
        int socketServerClient = conectSocketServer(client->port, client->ip);
        
        //Solicita arquivo
        sendInt(GET_FILE_COMMAND, socketServerClient);
        sendString(fileName, socketServerClient);

        //Monta modelo de nome de arquivo
        char modelFileName [MAXRCVLEN];
        char posfix[] = "_client%d_%d%d%d_%d%d.";
        int salto = 0;
        for (int i = 0; i < strlen(fileName); i++){
            if (fileName[i] == '.'){
                modelFileName[i] = '\0';
                strcat(modelFileName, posfix);
                salto = strlen(posfix) - 1;
            }
            else
                modelFileName[i + salto] = fileName[i];
        }
        modelFileName[strlen(fileName) + salto] = '\0';

        time_t t = time(NULL);
        struct tm tm = *localtime(&t);

        char newFileName [MAXRCVLEN];
        sprintf(newFileName, modelFileName, client->idClient, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min);

        //Montando path file
        char pathFile[strlen(directory) + strlen(newFileName) + 1];
        strcpy(pathFile, directory); 
        strcat(pathFile, newFileName);

        //Recebe arquivo
        recvFile(pathFile, socketServerClient);
        close(socketServerClient);

        //Atualiza estrutura do server
        sendInt(ADD_FILE_CLIENT_COMMAND, socketServer);
        sendInt(myId, socketServer);
        sendString(newFileName, socketServer);

        printf(" Arquivo %s baixado, nomeado como %s \n", fileName, newFileName);
        return (void *) EXIT_SUCCESS;
    }
}

void getFileClient(int socket, char * directory, int idClient){
    
    char delimiter = ' ';
    int nFiles = 0;
    char ** files = NULL;

    //Obtem lista de arquivos desejados
    while (delimiter != '\n'){

        files = (char**) realloc(files, (nFiles + 1) * sizeof(char*));
        files[nFiles] = (char*) calloc((MAXRCVLEN + 1), sizeof(char));

        scanf("%s", files[nFiles]);
        delimiter = getchar();
        nFiles++;
    }

    pthread_t * threadFile = NULL;
    for (int i = 0; i < nFiles; i++){

        //Obtem dados do cliente com o arquivo
        sendInt(GET_CLIENT_CONECT_COMMAND, socket);
        sendString(files[i], socket);
        Client * client = recvClientInfo(socket);

        //Cria thread para transferencia
        ArgsGetFileClientAndSave * args = (ArgsGetFileClientAndSave *) calloc(1, sizeof(ArgsGetFileClientAndSave));
        args->socket = socket;
        args->idClient = idClient;
        args->fileName = files[i];
        args->directory = directory;
        args->client = client;
        
        threadFile = (pthread_t*) calloc(1, sizeof(pthread_t));
        pthread_create(threadFile, NULL, getFileClientAndSave, args);  
    } 
}



int main(int argc, char *argv[])
{
	if( argc < 5 ){ 
        printf("ATENCAO: quantidade de argumentos insuficiente.\n");
        return EXIT_FAILURE;
    }

    //Inicialização de variáveis
    char * directory    = argv[1];
    int portClient      = atoi(argv[2]);
    char * ipServer     = argv[3];
    int portServer      = atoi(argv[4]);

    //Conexão com servidor
    int socketServer = conectSocketServer(portServer, ipServer);
    if (socketServer < 0){
        printf("FALHA: Erro ao se conectar ao servidor - %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    //Configurando endereço do mini-servidor
    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));             
    serv.sin_family = AF_INET;                 
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(portClient);

    //Realizando sincronização de arquivos disponíveis
    char * ipClient = getMyLocalIP();
    int idClient = sendClientInfo(socketServer, portClient, ipClient, directory);

    //Criação de thread para mini-servidor
    ArgsInitClientServer * args = (ArgsInitClientServer *) calloc(1, sizeof(ArgsInitClientServer));
    args->clientServ = serv;
    args->directory = directory;
    args->socketServer = socketServer;

    pthread_t * threadClient = (pthread_t*) calloc(1, sizeof(pthread_t));
    pthread_create(threadClient, NULL, initClientServer, args);
                
    //Laço de comandos
    while (true)
    {
        char command[500];
        printf("> ");
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

            case GET_FILE_COMMAND:
                getFileClient(socketServer, directory, idClient);
                break;
        }
        
    }

    return EXIT_SUCCESS;
}
