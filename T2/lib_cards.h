#ifndef LIB_CARDS_H
#define LIB_CARDS_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define USADA   127
#define MORTO   255

#define OUROS   0
#define ESPADAS 1
#define COPAS   2
#define PAUS    3

#define MAX_CARTAS_MAO 9
#define TAM_BARALHO 40
#define VIDAS_MAX 4

struct carta_t{
    unsigned int num;
    unsigned int naipe;
};

/* Escolho aleatoriamente uma carta do baralho e a retorno */
struct carta_t carta_aleatoria(unsigned int *baralho);

/* Marco a carta *c como utilizada*/
void marcar_carta_usada(struct carta_t *c);

/* Gero no vetor v de carta_t, n cartas aleatorias do baralho */
void gera_cartas_aleatorias(struct carta_t *v, unsigned int *baralho, unsigned int n);

void print_deck(struct carta_t *v, unsigned int n);

/* A partir dos dados data, de tamanho n, crio um vetor de carta de num_cards */
/* Contendo as cartas representadas nos dados data */
struct carta_t *vetor_cartas(char *data, unsigned int n, uint8_t num_cards);

/* Converto as n cartas em struct carta_t *c para uma string str. Armazeno */
/* o tamanho dessa string em size */
void mao_baralho(struct carta_t *c, int n, unsigned int *size, char *str);

/* Checo no vetor de vidas de tamanho n se há apenas um jogador vivo. */
/* Retorno 1 em aso postivo e 0 em caso negativo */
int unico_sobrevivente(uint8_t *vidas, uint8_t n);

/* Calcula o jogador vitorioso da rodada (r), e marca o contador de vitórias em v[i] */
/* Recebe como parametro o numero de jogadores (n) e a carta mais forte da rodada (gato) */
uint8_t calcula_vitoria(struct carta_t *r, uint8_t *v, unsigned int n, struct carta_t gato);

/* Converte as apostas contidas na string data de tamanho n para sua represntação */
/* inteira no vetor a de tamanho k */
void converte_apostas(char *data, unsigned int n, uint8_t *a, unsigned int k);

/* Converte as cartas jogadas na rodada, na string data de tamanho n, para sua representação */
/* no vetor carta_t r de tamanho k */
void converte_rodada(char *data, unsigned int n, struct carta_t *r, unsigned int k);

/* Desconto de cada indice do vetor vidas de tamanho n, o numero de vidas perdidas */
/* O calculo é vidas[i] = apostas[i] - vitorias[i] caso haja mais apostas que vitorias */
/* vidas[i] = vitorias[i] - apostas[i] caso contrário */
/* Se o desconto for maior que o numero de vidas[i], vidas[i] = 0*/
void descontar_vidas_perdidas(uint8_t *vidas, uint8_t *apostas, uint8_t *vitorias, uint8_t n);

/* A partir da string data e tamanho n, retorna o número de vidas que o jogador de numero index possui */
/* Ao fim da partida com mao de maxHand. Imprime também a diferença entre o valor final de vidas e checkpointVidas*/
int vida_final_partida(char *data, unsigned int n, int checkpointVidas, unsigned int index, unsigned int maxHand);

/* Imprime todas as n apostas na partida com mao maxHand*/
void print_apostas(char *apostas, unsigned int n , int maxHand);

/* Imprime as n cartas em struct carta_t mao, na rodada round*/
void print_mao(struct carta_t *mao, unsigned int n, unsigned int round);

/* Imprime as cartas que estão na mesa, onde os dados estao na string data de tamanho n*/
void print_mesa(char *data, unsigned int n);

/* Imprime o resultado da rodada na string data de tamanho n na rodada round*/
void print_resultado_rodada(char *data, unsigned int n, int round);

/* Imprime o resultado da rodada na string data de tamanho n ao jogar com a mao de cartas maxHand*/
void print_resultado_partida(char *data, unsigned int n, int maxHand);

/* Imprime o resultado do jogo na string data de tamanho n ao alcaçar a mão maxHand*/
void print_fim_jogo(char *data, unsigned int n, int maxHand);

/* Coleta a aposta do jogador ao jogar com a mão com tamanho de round */
int apostar(int round);

/* Coleta a escolha da carta presente no vetor v com n cartas ao todo */
int escolhe_cartas(struct carta_t *v, int n);

/* Converto o inteiro i para sua representação em char do numero do baralho */
char converte_numero_baralho(unsigned int i);

/* Converto o inteiro i para sua representação em char do naipe baralho */
char converte_numero_naipe(unsigned int i);

/* Converto o char i para a representação inteira do numero do baralho */
int converte_char_baralho(char i);

/* Converto o char i para a representação inteira do naipe do baralho */
int converte_char_naipe(char i);

/* Converto o char i (entre 0 e 9) para sua representação em inteiro */
int converte_char_int(char i);

/* Converto o inteiro i (entre 0 e 9) para sua representação em char */
char converte_int_char(int i);

/* Converto o char i para a representação em string do naipe do baralho */
char *converte_char_naipe_string(char i);

/* Converto o inteiro i para a representação em string do naipe do baralho */
char *converte_int_naipe_string(int i);

void header_jogo_dane_se();

#endif //#ifndef LIB_CARDS_H
