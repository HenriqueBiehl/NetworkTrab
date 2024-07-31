#include "lib_cards.h"
#include <stdlib.h>

struct carta_t carta_aleatoria(unsigned int *baralho){
    struct carta_t c;
    int r;

    r = rand()%40;
    printf("R:%d\n", r);
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

char converte_numero_baralho(unsigned int i){

    switch(i){
        case 0:
            return '4';
            break; 
        case 1:
            return '5';
            break; 
        case 2:
            return '6';
            break; 
        case 3:
            return '7';
            break; 
        case 4: 
            return 'Q';
            break;
        case 5: 
            return 'J';
            break;
        case 6: 
            return 'K';
            break;
        case 7: 
            return 'A';
            break;
        case 8: 
            return '2';
            break;
        case 9: 
            return '3';
            break;
    }

    return '\0';
}

char converte_numero_naipe(unsigned int i){

    switch(i){
        case 0:
            return 'O';
            break; 
        case 1:
            return 'E';
            break; 
        case 2:
            return 'C';
            break; 
        case 3:
            return 'P';
            break;
    }

    return '\0';
}

int converte_char_baralho(char i){

    switch(i){
        case '4':
            return 0;
            break; 
        case '5':
            return 1;
            break; 
        case '6':
            return 2;
            break; 
        case '7':
            return 3;
            break; 
        case 'Q': 
            return 4;
            break;
        case 'J': 
            return 5;
            break;
        case 'K': 
            return 6;
            break;
        case 'A': 
            return 7;
            break;
        case '2': 
            return 8;
            break;
        case '3': 
            return 9;
            break;
    }

    return -1;
}

int converte_char_naipe(char i){

    switch(i){
        case 'O':
            return 0;
            break; 
        case 'E':
            return 2;
            break; 
        case 'C':
            return 3;
            break; 
        case 'P':
            return 4;
            break;
    }

    return -1;
}
