#ifndef LIB_TOKEN_RING_H
#define LIB_TOKEN_RING_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#include "lib_cards.h"

#define PORT_BASE 5000
#define BUF_SIZE 256
#define NUM_NODES 4
#define MAX_DATA_LENGHT 2048

#define TOKEN_RING_SIZE 66
#define TOKEN_SIZE 64
#define FRAME_SIZE 2058


#define START 0x7e
#define SHUFFLE_FLAG 0 
#define BET_FLAG 1
#define MATCH_FLAG 2 
#define RESULTS_FLAG 3
#define ALL_BETS_FLAG 4
#define MATCH_RESULTS_FLAG 5
#define END_GAME_FLAG 255


#define LOCAL_PORT 12346                  // Porta local de recepção


struct message_frame{
    uint8_t start;                        //Bits de inicio transmissão
    unsigned int size;                    //Define o tamanho do campo *data
    uint8_t dest;                         //Define o índice de destino
    uint8_t flag;                         //Flags que definem o "momento do jogo" (embaralhamento, aposta, partida, resultados)
    uint8_t round;                        //Determina em qual rodada está o jogo 
    uint8_t num_cards;                    //Informa quantas cartas estão presentes na rodada
    char data[MAX_DATA_LENGHT+1];         //Campo de dados onde são enviados os dados da partida 
};

struct token_ring{
    uint8_t start;                        //Bits de inicio transmissão
    char token[TOKEN_SIZE+1];
};

/* Realiza bind no socket sock, marcando em addr o uso da porta port */
int bind_socket(int sock, struct sockaddr_in *addr, unsigned int port);

/* Seta o nodo de envio de mensagem com o endereço ip_next_node utlizando a porta port */
void setar_nodo_mult_maquinas(struct sockaddr_in *node, char *ip_next_node, unsigned short port);

/* Seta o nodo de envio de mensagem utlizando a porta definida pela constante PORT_BASE+index*/
void setar_nodo_loop_back(struct sockaddr_in *node, unsigned int index);

/* Inicializa o token da rede */
struct token_ring incializa_token();

/* Prepara o frame de mensagem frame com os parametros de transmissão do mesmo*/
void preparar_mensagem(struct message_frame *frame, char *data, unsigned int size, int flag, int round, int num_cards, uint8_t dest);

/* Gera mensagem de resultado da rodada na string data, salvando o tamanho em tam, utilizando as cartas da rodada em r de tamanho n*/
void gera_mensagem_resultado(char *data, unsigned int *tam, struct carta_t *r, uint8_t n, uint8_t ganhador, struct carta_t gato);

/* Gera mensagem de resultado da partida na string data, salvando o tamanho em tam, utilizando dos dados de apostas, vitorias e vidas, todos de tam n*/
void gera_mensagem_partida(char *data, unsigned int *tam, uint8_t *apostas, uint8_t *vitorias, uint8_t *vidas , uint8_t n);

/* Gera mensagem de resultado de fim de jogo na string data, salvando o tamanho em tam, utilizando do dado das vidas de tam n*/
void gerar_mensagem_fim_jogo(char *data, unsigned int *tam, uint8_t *vidas, unsigned int n);

/* Recebe o token no socket sock, armazenando os dados em node_token, recebendo do endereço de from_addr */
void receber_token(int sock, struct token_ring *node_token ,struct sockaddr_in from_addr, socklen_t addr_size);

#endif //LIB_TOKEN_RING_H
