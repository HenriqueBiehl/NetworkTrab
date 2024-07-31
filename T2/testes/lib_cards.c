#include "lib_cards.h"
#include <stdlib.h>

struct carta_t carta_aleatoria(unsigned int *baralho){
    struct carta_t c;
    int r;

    r = rand()%40;

    while(baralho[r]){
        r = rand()%40;
    }
    baralho[r] = 1;

    c.num = r % 10; 
    c.naipe = r/10;

    return c;
}

void gera_cartas_aleatorias(struct carta_t *v, unsigned int *baralho, unsigned int n){

    for(int i=0; i < n; ++i){
        v[i] = carta_aleatoria(baralho);
    }
}

