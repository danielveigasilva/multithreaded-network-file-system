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

#define MAXRCVLEN 500

int main(int argc, char *argv[])
{

	if( argc != 2 ){ 
        printf("USAGE: server port_number\n");
        return EXIT_FAILURE;
    }

   char buffer[MAXRCVLEN + 1]; /* +1 so we can add null terminator */
   bzero( buffer, MAXRCVLEN + 1 );
   int len, mysocket;
   struct sockaddr_in dest;

   mysocket = socket(AF_INET, SOCK_STREAM, 0);

   memset(&dest, 0, sizeof(dest));                /* zero the struct */
   dest.sin_family = AF_INET;
   dest.sin_addr.s_addr = htonl(INADDR_LOOPBACK); /* set destination IP number - localhost, 127.0.0.1*/
   dest.sin_port = htons(  atoi( argv[ 1 ]) );                /* set destination port number */

   int connectResult = connect(mysocket, (struct sockaddr *)&dest, sizeof(struct sockaddr_in));

   if( connectResult == - 1 ){
   		printf("CLIENT ERROR: %s\n", strerror(errno));
   		return EXIT_FAILURE;
   }

   strcpy(buffer,recvString(mysocket));

   /* We have to null terminate the received data ourselves */
   //buffer[len] = '\0';

   printf("Received %s (%d bytes).\n", buffer, len);

   close(mysocket);
   return EXIT_SUCCESS;
}
