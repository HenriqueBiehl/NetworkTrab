#include "network.h"

void printBinary(uint8_t n) {
        // Define the number of bits in an unsigned int
        unsigned int numBits = sizeof(uint8_t) * 8;

        // Iterate through each bit (from the most significant to the least significant)
        for (int i = numBits - 1; i >= 0; i--) {
                // Extract the i-th bit and print it
                unsigned int bit = (n >> i) & 1;
                printf("%u", bit);
        }

        printf("\n"); // Move to the next line after printing the binary representation
}

void printFrame(struct networkFrame frame) {

        printBinary(frame.start);
        printf("Size: %u ", frame.size);
        printf("\n");
        printf("Seq: %u ", frame.seq); 
        printf("\n");
        printf("Type: %u ", frame.type);
        printf("\nData: ");
        for(int i=0; i < MAX_DATA_LENGHT; ++i)
                printf("%c ", frame.data[i]);
        printf("\n");
        printf("CRC: %u ", frame.crc8); 
}
