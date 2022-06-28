/**
 * @file biblioteca.h
 *
 * Este cabeçalho define as funções para manipulação de memória.
 */

#define LIST_COMMAND 0
#define STATUS_COMMAND 1
#define SEND_FILENAMES_COMMAND 3
#define true 1
#define false 0

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
