#include <stdio.h>
#include <sys/mman.h> 
#include <fcntl.h> 
#include <unistd.h>
#define _BSD_SOURCE

#include <sys/time.h> // sanity

#define RADIO_TUNER_FAKE_ADC_PINC_OFFSET 0
#define RADIO_TUNER_TUNER_PINC_OFFSET 1
#define RADIO_TUNER_CONTROL_REG_OFFSET 2
#define RADIO_TUNER_TIMER_REG_OFFSET 3
#define RADIO_PERIPH_ADDRESS 0x43c00000

// Divide by 4 because ptr is 4byte
#define FIFO_LEN_OFFSET 0x24/4
#define FIFO_READ_OFFSET 0x20/4
#define FIFO_PERIPH_ADDRESS 0x43c10000

#define WORDS_TO_RECEIVE_TOTAL 480000
#define WORDS_PER_PACKET 256
#define BYTES_PER_PACKET WORDS_PER_PACKET*4

// the below code uses a device called /dev/mem to get a pointer to a physical
// address.  We will use this pointer to read/write the custom peripheral
volatile unsigned int * get_a_pointer(unsigned int phys_addr)
{

	int mem_fd = open("/dev/mem", O_RDWR | O_SYNC); 
	void *map_base = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, phys_addr); 
	volatile unsigned int *radio_base = (volatile unsigned int *)map_base; 
	return (radio_base);
}

// Radio
void radioTuner_tuneRadio(volatile unsigned int *ptrToRadio, float tune_frequency)
{
	float pinc = (-1.0*tune_frequency)*(float)(1<<27)/125.0e6;
	*(ptrToRadio+RADIO_TUNER_TUNER_PINC_OFFSET)=(int)pinc;
}

void radioTuner_setAdcFreq(volatile unsigned int* ptrToRadio, float freq)
{
	float pinc = freq*(float)(1<<27)/125.0e6;
	*(ptrToRadio+RADIO_TUNER_FAKE_ADC_PINC_OFFSET) = (int)pinc;
}

// FIFO
unsigned int get_fifo_len(volatile unsigned int *fifo)
{
	unsigned int len = *(fifo+FIFO_LEN_OFFSET);
    // RLR is lower 23 bits
    len = len & 0x7FFFFF;
    return len;
}

unsigned int read_fifo(volatile unsigned int *fifo)
{
	unsigned int data = *(fifo+FIFO_READ_OFFSET);
    return data;
}

// Program
void receive_data(volatile unsigned int *fifo)
{
    printf("Starting to receive data\n");
    unsigned int data, available;
    for (int received_words=0; received_words<WORDS_TO_RECEIVE_TOTAL; ) {
        // Check if we have enough data to receive. RLR (length) is in bytes!
        if ( (available = get_fifo_len(fifo)) < BYTES_PER_PACKET)
        { /* printf("Not enough, %u\n", available); */ continue; }
        printf("%u available\n", available);

        // Receive
        for (int i=0; i<WORDS_PER_PACKET; i++)
        {
            data = read_fifo(fifo);
        }

        // Increment count
        received_words += WORDS_PER_PACKET;

        printf("Got enough data for a packet. Last byte: %u\n", data);
    }
    printf("Got enough data forever. Done\n");
}

int main()
{

    volatile unsigned int *radio = get_a_pointer(RADIO_PERIPH_ADDRESS);	
    volatile unsigned int *fifo = get_a_pointer(FIFO_PERIPH_ADDRESS);
    printf("\r\n\r\n\r\nLab 10 Daniel Mirsky - FIFO Test\n\r");

    // Setup
    *(radio+RADIO_TUNER_CONTROL_REG_OFFSET) = 0; // make sure radio isn't in reset
    printf("Tuning Radio to 30MHz\n\r");
    radioTuner_tuneRadio(radio,30e6);
    printf("Tuning ADC to 30.001MHz\n\r");
    radioTuner_setAdcFreq(radio,30.001e6);

    // Receive Data and time
    struct timeval start_tv, end_tv;
    gettimeofday(&start_tv, NULL);
    receive_data(fifo);
    gettimeofday(&end_tv, NULL);
    printf("\n\nElapsed time in whole sec = %u\n", end_tv.tv_sec - start_tv.tv_sec);

    return 0;
}
