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

void marcar_carta_usada(struct carta_t *c){
    c->num = USADA;
}

void gera_cartas_aleatorias(struct carta_t *v, unsigned int *baralho, unsigned int n){

    for(int i=0; i < n; ++i){
        v[i] = carta_aleatoria(baralho);
    }
}

void print_deck(struct carta_t *v, unsigned int n){

    for(int i=0; i < n; ++i){
        if(v[i].num != 10)
            printf("[%d] - %c de %c| ", i+1, converte_numero_baralho(v[i].num), converte_numero_naipe(v[i].naipe));
    }
    printf("\n");
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
            return 1;
            break; 
        case 'C':
            return 2;
            break; 
        case 'P':
            return 3;
            break;
    }

    return -1;
}

int converte_char_int(char i){
    
    switch(i){
        case '0':
            return 0;
            break;
        case '1':
            return 1;
            break; 
        case '2':
            return 2;
            break; 
        case '3':
            return 3;
            break; 
        case '4':
            return 4;
            break;
        case '5':
            return 5;
            break;
        case '6':   
            return 6;
            break;
        case '7':
            return 7; 
            break;
        case '8':
            return 8; 
            break;
        case '9':
            return 9;
            break;
    }

    return -1;
}

char converte_int_char(int i){
    
    switch(i){
        case 0:
            return '0';
            break;
        case 1:
            return '1';
            break; 
        case 2:
            return '2';
            break; 
        case 3:
            return '3';
            break; 
        case 4:
            return '4';
            break;
        case 5:
            return '5';
            break;
        case 6:   
            return '6';
            break;
        case 7:
            return '7'; 
            break;
        case 8:
            return '8'; 
            break;
        case 9:
            return '9';
            break;
    }

    return -1;
}