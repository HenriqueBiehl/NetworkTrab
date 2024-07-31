#include <stdio.h>

struct carta_t{
    unsigned int num;
    unsigned int naipe;
};


struct carta_t carta_aleatoria(unsigned int *baralho);

void gera_cartas_aleatorias(struct carta_t *v, unsigned int *baralho, unsigned int n);