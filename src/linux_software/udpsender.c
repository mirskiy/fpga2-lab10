// Based on https://www.cs.cmu.edu/afs/cs/academic/class/15213-f99/www/class26/udpclient.c
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>

#define BUFSIZE 1028
#define DATASIZE 256
#define PORT 25344

void error(char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char **argv) {
    // Socket and server info
    int sockfd;
    struct sockaddr_in serveraddr;
    const struct sockaddr *serveraddr_p = &serveraddr;
    int serverlen = sizeof(serveraddr);
    // result for error checking
    int res; 
    // data buffer
    char buf[BUFSIZE] = "     test";
    uint32_t *const counter = &buf[0];
    uint32_t *data = &buf[4];

    // Inputs
    char *addr;
    int count;

    /* check command line arguments */
    if (argc != 3) {
       fprintf(stderr, "usage: %s <hostname> <count>\n", argv[0]);
       exit(0);
    }
    addr = argv[1];
    count = atoi(argv[2]);

    /* set buffer for debugging */
    memset(buf, 55, BUFSIZE);

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(addr);
    serveraddr.sin_port = htons(PORT);

    /* send the message to the server */
    for (uint32_t i=0; i<count; i++) {
        *counter = i;
        for (uint32_t j=0; j<DATASIZE; j++)
        { data[j] = i*DATASIZE+j; }
        //*counter = 1;
        //data[0] = 0x11111111;
        //data[1] = 0x22222222;
        //printf("%x %x %x %x", data, data+1, &(data[0]), &(data[1]));
        res = sendto(sockfd, buf, BUFSIZE, 0, serveraddr_p, serverlen);
        if (res < 0) 
          error("ERROR in sendto");
    }
    
    return 0;
}
