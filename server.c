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

int main(int argc, char *argv[])
{

    if( argc != 2 ){ 
        printf("USAGE: server port_number\n");
        return EXIT_FAILURE;
    }

    char* msg[5] = {"Homem-Animal\n", "Mulher-Hulk\n", "MiracleMan\n", "V de Vingaça\n", "Watchmen\n"};

    srand( time( NULL ));

    struct sockaddr_in dest; /* socket info about the machine connecting to us */
    struct sockaddr_in serv; /* socket info about our server */
    int mysocket;            /* socket used to listen for incoming connections */
    socklen_t socksize = sizeof(struct sockaddr_in);

    memset(&serv, 0, sizeof(serv));           /* zero the struct before filling the fields */
    serv.sin_family = AF_INET;                /* set the type of connection to TCP/IP */
    serv.sin_addr.s_addr = htonl(INADDR_ANY); /* set our address to any interface */
    serv.sin_port = htons( atoi( argv[ 1 ] ) );           /* set the server port number */

    mysocket = socket(AF_INET, SOCK_STREAM, 0);

    /* bind serv information to mysocket */
    bind(mysocket, (struct sockaddr *)&serv, sizeof(struct sockaddr));

    /* start listening, allowing a queue of up to 1 pending connection */
    listen(mysocket, 1);


    printf("Server is waiting for connections on port:%s\n", argv[ 1 ] );

    int consocket = accept(mysocket, (struct sockaddr *)&dest, &socksize);

    while(consocket){
        printf("Incoming connection from %s - sending welcome\n", inet_ntoa(dest.sin_addr));

        int randomMessage = rand() % 3;

        sendString(msg[randomMessage], consocket);
        close(consocket);
        consocket = accept(mysocket, (struct sockaddr *)&dest, &socksize);
    }

    close(mysocket);
    return EXIT_SUCCESS;
}
