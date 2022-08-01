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
    int idClient;
    char * directory;
}ArgsInitClientServer;


typedef struct ArgsProcessCommandsFromOtherClient_t{
    int socketClient;
    int socketServer;
    int idClient;
    char * directory;
}ArgsProcessCommandsFromOtherClient;


typedef struct ArgsGetFileClientAndSave_t{
    int socket;
    int idClient;
    char * fileName;
    char * directory;
    Client * client;
}ArgsGetFileClientAndSave;

typedef struct ArgsSendFileToClientInThread_t{
    char * fileName;
    char * directory;
    int socketClient;
    int idClient;
} ArgsSendFileToClientInThread;



int mapCommand(char* command){
    if (!strcmp(command, "list"))
        return LIST_COMMAND;
    if (!strcmp(command, "status"))
        return GET_STATUS_COMMAND;
    if (!strcmp(command, "rm"))
        return DELETE_FILE_CLIENT_COMMAND;
    if (!strcmp(command, "get"))
        return GET_FILE_COMMAND;
    if (!strcmp(command, "send"))
        return SEND_FILE_COMMAND;
    if (!strcmp(command, "exit"))
        return DELETE_CLIENT_COMMAND;
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
            nameFiles[countFiles] = (char*) calloc((sizeof(entry->d_name) + 1), sizeof(char));
            strcpy(nameFiles[countFiles], entry->d_name);
            
            countFiles++;
        }

        sendInt(countFiles, socket);

        for (int i = 0; i < countFiles; i++)
            sendString(nameFiles[i], socket);
    }
} 


Client * recvClient(int socket){

    Client * client = calloc(1, sizeof(Client));

    client->ip = recvString(socket);
    client->port = recvInt(socket);
    client->idClient = recvInt(socket);

    return client;
}

Client* recvClientInfo(int socket){
    
    int status = recvInt(socket);
    if (status == STATUS_NOT_FOUND)
        return NULL;

    Client * client = recvClient(socket);
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

void recvFileClient(char * directory, int socketClient, int socketServer, int myId){
    char * fileName = recvString(socketClient);
    int idClientSend = recvInt(socketClient);

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
    sprintf(newFileName, modelFileName, idClientSend, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min);

    //Montando path file
    char pathFile[strlen(directory) + strlen(newFileName) + 1];
    strcpy(pathFile, directory); 
    strcat(pathFile, newFileName);

    //Recebe arquivo
    recvFile(pathFile, socketClient);
    close(socketClient);

    //Atualiza estrutura do server
    sendInt(ADD_FILE_CLIENT_COMMAND, socketServer);
    sendInt(myId, socketServer);
    sendString(newFileName, socketServer);

    printf(" Arquivo %s baixado, nomeado como %s \n", fileName, newFileName);
}

void * processCommandsFromOtherClient(void * args){
    ArgsProcessCommandsFromOtherClient * argumentos = (ArgsProcessCommandsFromOtherClient*) args; 
    
    int socketClient = argumentos->socketClient;
    int socketServer = argumentos->socketServer;
    int idClient = argumentos->idClient;
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
            
            case SEND_FILE_COMMAND:
                recvFileClient(directory, socketClient, socketServer, idClient);
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
        args->idClient = argumentos->idClient;
        
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

int sendClientInfo(int socket, int portClient, char * directory){
    sendInt(SEND_CLIENT_INFO_COMMAND, socket);
    
    sendInt(portClient, socket);
    sendClientFileNames(directory, socket);

    int idClient = recvInt(socket);
    return idClient;
}

void deleteFileClient(int socket){

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

    for (int i = 0; i < nFiles; i++){

        //Obtem dados do cliente com o arquivo
        sendInt(GET_CLIENT_BY_FILE_COMMAND, socket);
        sendString(files[i], socket);
        Client * client = recvClientInfo(socket);

        if (client == NULL)
            printf(" FALHA: Arquivo %s não localizado.\n", files[i]);
        else {
            //Conecta ao server do cliente
            int socketServerClient = conectSocketServer(client->port, client->ip);
            
            //Solicita delete arquivo
            sendInt(DELETE_FILE_CLIENT_COMMAND, socketServerClient);
            sendString(files[i], socketServerClient);

            close(socketServerClient);
        }

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
        sendInt(GET_CLIENT_BY_FILE_COMMAND, socket);
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

void deleteClient(int socket, int idClient){
    sendInt(DELETE_CLIENT_COMMAND, socket);
    sendInt(idClient, socket);
    close(socket);
}

void getStatus(int socket){
    sendInt(GET_STATUS_COMMAND, socket);
    int nClients = recvInt(socket);

    for (int i = 0; i < nClients; i++){
        Client * client = recvClient(socket);
        printf("\n");
        printf(" Cliente - %d\n", client->idClient);
        printf("  # %s:%d\n", client->ip, client->port);
    }
    printf("\n");
}

void * sendFileToClientInThread(void * _args){

    ArgsSendFileToClientInThread * args = (ArgsSendFileToClientInThread *) _args;
    int socket = args->socketClient;
    int idClient = args->idClient;

    //Montando path file
    char * fileName = args->fileName;
    char * directory = args->directory;
    char pathFile[strlen(directory) + strlen(fileName) + 1];
    strcpy(pathFile, directory); 
    strcat(pathFile, fileName);

    //Envia arquivo
    if (access(pathFile, F_OK) == 0){

        sendInt(SEND_FILE_COMMAND, socket);
        sendString(fileName, socket);
        sendInt(idClient, socket);

        sendFile(pathFile, args->socketClient);
    }
    else
        printf(" FALHA: Arquivo %s não localizado em Client %d\n",fileName, idClient);

    return (void *) 0;
}

void sendFilesToClient(int socket, char * dir, int myId){

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

    int idClientDest = atoi(files[nFiles-1]); // id client destino ultimo argumento

    //Obtem dados do cliente de destino
    sendInt(GET_CLIENT_BY_ID_COMMAND, socket);
    sendInt(idClientDest, socket);
    Client * client = recvClientInfo(socket);

    if (client == NULL)
        printf(" FALHA: Cliente %d nao localizado.\n", idClientDest);
    else {
        for (int i = 0; i < nFiles - 1; i++){
            
            int socketMiniServer = conectSocketServer(client->port, client->ip);
            
            if (socketMiniServer == -1)
                printf(" FALHA: não foi possivel se conectar ao Cliente %d.\n", idClientDest);
            else{
                 //Criação de thread para tranferencia de arquivos
                ArgsSendFileToClientInThread * args = (ArgsSendFileToClientInThread *) calloc(1, sizeof(ArgsSendFileToClientInThread));
                args->fileName = files[i];
                args->directory = dir;
                args->socketClient = socketMiniServer;
                args->idClient = myId;

                pthread_t * threadClient = (pthread_t*) calloc(1, sizeof(pthread_t));
                pthread_create(threadClient, NULL, sendFileToClientInThread, args);   
            }
        }
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
    serv.sin_addr.s_addr = htonl(INADDR_ANY);
    serv.sin_port = htons(portClient);

    //Realizando sincronização de arquivos disponíveis
    int idClient = sendClientInfo(socketServer, portClient, directory);

    //Criação de thread para mini-servidor
    ArgsInitClientServer * args = (ArgsInitClientServer *) calloc(1, sizeof(ArgsInitClientServer));
    args->clientServ = serv;
    args->directory = directory;
    args->socketServer = socketServer;
    args->idClient = idClient;

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
            case LIST_COMMAND: //Lista arquivos disponíveis no server
                recvFilesNames(socketServer);
                break;

            case DELETE_FILE_CLIENT_COMMAND: //Solicita delete de arquivos
                deleteFileClient(socketServer);
                break;

            case GET_FILE_COMMAND: //Baixa arquivos
                getFileClient(socketServer, directory, idClient);
                break;

            case GET_STATUS_COMMAND: //Obtem status do server
                getStatus(socketServer);
                break;

            case SEND_FILE_COMMAND: //Envia arquivos
                sendFilesToClient(socketServer, directory, idClient);
                break;

            case DELETE_CLIENT_COMMAND: //Desconecta do server
                deleteClient(socketServer, idClient);
                return EXIT_SUCCESS; //Finaliza programa
                break;

            default:
                printf("ERRO: Comando '%s' inválido\n", command);
                break;
        }
        
    }

    return EXIT_SUCCESS;
}
