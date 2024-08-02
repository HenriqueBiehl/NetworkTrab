#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h> 
#include <net/ethernet.h> 
#include <linux/if_packet.h> 
#include <net/if.h> 
#include <stdint.h>

#define MAX_DATA_LENGHT 63
#define FRAME_SIZE 67

#define START 0x7e

#define ACK 0
#define NACK 1
#define LISTA 10 
#define BAIXAR 11
#define MOSTRA_NA_TELA 16
#define DESCRITOR_ARQUIVO 17
#define DADOS 18
#define FIM_TX 30
#define ERRO 31

#define DIR_CONTEUDOS "conteudos"

struct __attribute__((packed)) networkFrame {
        uint8_t start; 
        uint8_t size:6;
        uint8_t seq:5;
        uint8_t type:5;
        unsigned char data[MAX_DATA_LENGHT];  
        uint8_t crc8;
};

void printBinary(uint8_t n);

void printFrame(struct networkFrame frame);

uint8_t calcula_crc8(uint8_t *data, size_t len);

int verifica_crc8(uint8_t *data, size_t len, uint8_t crc_recebido);

FILE *abrir_arquivo(char *nome, char *tipo);

/*
        Lista os conteúdos de um diretório e envia para um arquivo temporário "lista"         
*/
void lista_conteudos();

/*
        Insere os dados de tamanho e data do arquivo "nome arquivo" 
        em um arquivo temporário "descritor"
*/
void descritor_arquivo(char *nomeArquivo);
