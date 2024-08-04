#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define USADA   127
#define MORTO   255

#define OUROS   0
#define ESPADAS 1
#define COPAS   2
#define PAUS    3

struct carta_t{
    unsigned int num;
    unsigned int naipe;
};


struct carta_t carta_aleatoria(unsigned int *baralho);

void marcar_carta_usada(struct carta_t *c);

void gera_cartas_aleatorias(struct carta_t *v, unsigned int *baralho, unsigned int n);

char converte_numero_baralho(unsigned int i);

void print_deck(struct carta_t *v, unsigned int n);

struct carta_t *vetor_cartas(char *data, unsigned int n, uint8_t num_cards);

void mao_baralho(struct carta_t *c, int n, unsigned int *size, char *str);

/* Calcula o jogador vitorioso da rodada (r), e marca o contador de vitórias em v[i] */
/* Recebe como parametro o numero de jogadores (n) e a carta mais forte da rodada (gato) */
/* ADICIONAR: calculo para caso o jogador i esteja morto */
uint8_t calcula_vitoria(struct carta_t *r, uint8_t *v, unsigned int n, struct carta_t gato);

void converte_apostas(char *data, unsigned int n, uint8_t *a, unsigned int k);

void converte_rodada(char *data, unsigned int n, struct carta_t *r, unsigned int k);

void descontar_vidas_perdidas(uint8_t *vidas, uint8_t *apostas, uint8_t *vitorias, uint8_t n);

int vida_final_partida(char *data, unsigned int n, int checkpointVidas, unsigned int index, unsigned int maxHand);

void print_apostas(char *apostas, unsigned int n , unsigned maxHand);

void print_mao(struct carta_t *mao, unsigned int n, unsigned int round);

void print_mesa(char *data, unsigned int n);

void print_resultado_rodada(char *data, unsigned int n, int round);

void print_resultado_partida(char *data, unsigned int n, int maxHand);

char converte_numero_naipe(unsigned int i);

int converte_char_baralho(char i);

int converte_char_naipe(char i);

int converte_char_int(char i);

char converte_int_char(int i);

