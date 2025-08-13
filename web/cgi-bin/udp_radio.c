#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>
#include <fcntl.h> 
#define _BSD_SOURCE

// UDP (Based on https://www.cs.cmu.edu/afs/cs/academic/class/15213-f99/www/class26/udpclient.c)
#define PORT 25344

// Radio
#define RADIO_TUNER_FAKE_ADC_PINC_OFFSET 0
#define RADIO_TUNER_TUNER_PINC_OFFSET 1
#define RADIO_TUNER_CONTROL_REG_OFFSET 2
#define RADIO_TUNER_TIMER_REG_OFFSET 3
#define RADIO_PERIPH_ADDRESS 0x43c00000

// FIFO
// Divide by 4 because ptr is 4byte
#define FIFO_LEN_OFFSET 0x24/4
#define FIFO_READ_OFFSET 0x20/4
#define FIFO_PERIPH_ADDRESS 0x43c10000

// Config
#define WORDS_PER_PACKET 256
#define BYTES_PER_PACKET WORDS_PER_PACKET*4

// ***** ***** Utility ***** *****
// *****
// Print Errors
void error(char *msg) {
    perror(msg);
    exit(0);
}

// Pointer to mem for interfaces
volatile unsigned int * get_a_pointer(unsigned int phys_addr)
{
	int mem_fd = open("/dev/mem", O_RDWR | O_SYNC); 
	void *map_base = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, phys_addr); 
	return (volatile unsigned int *)map_base; 
}


// ***** ***** FIFO ***** *****
// *****
// Get FIFO Length
unsigned int fifo_getLength(volatile unsigned int *fifo)
{
	unsigned int len = *(fifo+FIFO_LEN_OFFSET);
    // RLR is lower 23 bits
    len = len & 0x7FFFFF;
    return len;
}

// Read FIFO
uint32_t fifo_getData(volatile unsigned int *fifo)
{
	unsigned int data = *(fifo+FIFO_READ_OFFSET);
    return data;
}

// ***** ***** Radio ***** *****
// *****
// Tune Radio
void radio_tuneRadio(volatile unsigned int *ptrToRadio, float tune_frequency)
{
    printf("Tuning Radio to %fHz\n\r", tune_frequency);
	float pinc = (-1.0*tune_frequency)*(float)(1<<27)/125.0e6;
	*(ptrToRadio+RADIO_TUNER_TUNER_PINC_OFFSET)=(int)pinc;
}

// Tune ADC
void radio_setAdcFreq(volatile unsigned int* ptrToRadio, float freq)
{
    printf("Tuning ADC to %fHz\n\r", freq);
	float pinc = freq*(float)(1<<27)/125.0e6;
	*(ptrToRadio+RADIO_TUNER_FAKE_ADC_PINC_OFFSET) = (int)pinc;
}

// Get Packet
void radio_getPacket(volatile unsigned int *fifo, uint32_t *buffer)
{
    unsigned int available;

    // Check if we have enough data to receive. RLR (length) is in bytes!
    while ( (available = fifo_getLength(fifo)) < BYTES_PER_PACKET)
    { /* printf("Not enough, %u\n", available); */ continue; }
    //printf("%u available\n", available);

    // Receive
    for (int i=0; i<WORDS_PER_PACKET; i++)
    {
        buffer[i] = fifo_getData(fifo);
    }

    //printf("Got enough data for a packet. Last byte: %u\n", buffer[WORDS_PER_PACKET]);
}


// ***** ***** UDP ***** *****
// Setup socket
int udp_getSocket() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    return sockfd;
}
// Setup serveraddr
void udp_fillServeraddr(struct sockaddr_in *serveraddr_p, char *addr) {
    /* build the server's Internet address */
    bzero((char *) serveraddr_p, sizeof(*serveraddr_p));
    serveraddr_p->sin_family = AF_INET;
    serveraddr_p->sin_addr.s_addr = inet_addr(addr);
    serveraddr_p->sin_port = htons(PORT);
}

// Send Packet
void udp_sendPacket(int sockfd, const struct sockaddr *serveraddr_p,
                    unsigned int *buffer, size_t bytes_to_send ) {
    int serverlen = sizeof(*serveraddr_p);
    int res; 
    res = sendto(sockfd, buffer, bytes_to_send, 0, serveraddr_p, serverlen);
    if (res < 0) 
      error("ERROR in sendto");
    return;
}


// ***** ***** Main ***** *****
// args
char *main_checkArgs(int argc, char **argv){
    if (argc != 2) {
        fprintf(stderr, "usage: %s <hostname>\n", argv[0]);
        exit(0);
    }
    char *addr = argv[1];
    return addr;
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IOLBF, 0);
    printf("\r\n\r\n\r\nLab 10 Daniel Mirsky - FIFO Test\n\r");
    // Input
    char *addr = main_checkArgs(argc, argv);

    // UDP Setup: Socket and server info
    struct sockaddr_in serveraddr;
    udp_fillServeraddr(&serveraddr, addr);
    int sockfd = udp_getSocket();

    // Radio Seetup
    volatile unsigned int *radio = get_a_pointer(RADIO_PERIPH_ADDRESS);	
    volatile unsigned int *fifo = get_a_pointer(FIFO_PERIPH_ADDRESS);
    *(radio+RADIO_TUNER_CONTROL_REG_OFFSET) = 0; // make sure radio isn't in reset
    radio_tuneRadio(radio,  30e6);
    radio_setAdcFreq(radio, 30.001e6);

    // Data Setup
    int buffer_words = WORDS_PER_PACKET + 1; // +1 for counter
    size_t buffer_bytes = buffer_words * 4;
    uint32_t buffer[buffer_words];
    uint32_t *const counter = &buffer[0];
    uint32_t *const data = &buffer[1];
    memset(buffer, 0x55, buffer_bytes);  // For debugging

    *counter = 0;
    printf("Starting to receive data\n");
    while (1) {
        radio_getPacket(fifo, data);
        udp_sendPacket(sockfd, &serveraddr, buffer, buffer_bytes);
        (*counter)++;
    }

    return 0;
}
