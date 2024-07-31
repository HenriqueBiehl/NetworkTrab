#include <stdio.h>

struct carta_t{
    unsigned int num;
    unsigned int naipe;
};


struct carta_t carta_aleatoria(unsigned int *baralho);

void gera_cartas_aleatorias(struct carta_t *v, unsigned int *baralho, unsigned int n);

char converte_numero_baralho(unsigned int i);

char converte_numero_naipe(unsigned int i);

int converte_char_baralho(char i);

int converte_char_naipe(char i);
