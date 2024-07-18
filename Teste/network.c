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

uint8_t calcula_crc8(uint8_t *data, size_t len) {
    uint8_t crc = 0x00;
    uint8_t polynomial = 0x07; // Polinômio padrão do CRC-8: x^8 + x^2 + x + 1

    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ polynomial;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

int verifica_crc8(uint8_t *data, size_t len, uint8_t crc_recebido) {
    uint8_t crc_calculado = calcula_crc8(data, len);
    return (crc_calculado == crc_recebido);
}
