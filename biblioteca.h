/**
 * @file biblioteca.h
 *
 * Este cabeçalho define as funções para manipulação de memória.
 */

#define LIST_COMMAND 0
#define STATUS_COMMAND 1
#define SEND_CLIENT_INFO_COMMAND 3
#define CLIENT_INFO_COMMAND 4
#define DELETE_FILE_CLIENT_COMMAND 5
#define GET_FILE_COMMAND 6
#define GET_CLIENT_CONECT_COMMAND 7
#define ADD_FILE_CLIENT_COMMAND 8

#define FILE_BLOCK_SIZE 5

#define true 1
#define false 0

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
    unsigned int port;
    int ip;
    FileNameList * FileNameList;
    struct Client_t * nextClient;
}Client;

typedef struct ClientList_t{
    int nClients;
    Client * firstClient;
    Client * lastClient;
}ClientList;

/*! Envia valor do tipo Int
        @param value  Valor inteiro a ser enviado
        @param socket Socket de destino
        @return Tamanho da mensagem 
        */
void sendInt( int value, int socket );

/*! Recebe valor do tipo Int
        @param value  Valor int a ser enviado
        @param socket Socket do cliente
        @return Tamanho da mensagem 
        */
int recvInt( int socket );

/*! Envia valor do tipo Double
        @param value  Valor double a ser enviado
        @param socket Socket de destino
        @return Tamanho da mensagem 
        */
void sendDouble( double value, int socket );

/*! Recebe valor do tipo Double
        @param value  Valor double a ser recebida
        @param socket Socket do cliente
        @return Tamanho da mensagem 
        */
double recvDouble( int socket );

/*! Envia string
        @param value  string a ser enviada
        @param socket Socket de destino
        @return Tamanho da mensagem 
        */
void sendString( char* value, int socket );

/*! Recebe string
        @param value  string a ser recebida
        @param socket Socket do cliente
        @return Tamanho da mensagem 
        */
char* recvString( int socket );

/*! Envia Arquivo
        @param value  string a ser enviada
        @param socket Socket de destino
        @return Tamanho da mensagem 
        */
void sendFile( char * pathFile, int socket );

/*! Envia Arquivo
        @param value  string a ser enviada
        @param socket Socket de destino
        @return Tamanho da mensagem 
        */
void recvFile(char * pathNewFile, int socket );