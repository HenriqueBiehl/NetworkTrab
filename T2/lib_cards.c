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
        if(v[i].num != USADA)
            printf("[%d] - %c de %s\n", i+1, converte_numero_baralho(v[i].num), converte_int_naipe_string(v[i].naipe));
    }
}

struct carta_t *vetor_cartas(char *data, unsigned int n, uint8_t num_cards){
    struct carta_t *v; 
    unsigned int index = 0;
    
    v = malloc(num_cards*sizeof(struct carta_t));

    if(!v)
        return NULL;
    
    for(int i = 0; i < n && index < num_cards; i+=3){
        v[index].num = converte_char_baralho(data[i]); 
        v[index].naipe = converte_char_naipe(data[i+1]);
        index++;
    }

    return v;
} 

/* Converte o vetor deck para uma string formatada em cards.data. Recebe o ponteo de cards.size para salvar o tamanho*/
void mao_baralho(struct carta_t *c, int n, unsigned int *size, char *str){
    int msg_index = 0;
    struct carta_t x; 
    
    for(int i =0; i < n; ++i){
        x = c[i];
        str[msg_index] = converte_numero_baralho(x.num); 
        str[msg_index+1] = converte_numero_naipe(x.naipe);
        str[msg_index+2] = '|';
        msg_index += 3;
    }

    *size = msg_index;
    str[msg_index] = '\0';
}

void converte_apostas(char *data, unsigned int n, uint8_t *a, unsigned int k){
    unsigned int index = 0;

    for(int i=0; i < n && index < k; i+=2){
        a[index] = converte_char_int(data[i]);
        index++;
    }
}

void converte_rodada(char *data, unsigned int n, struct carta_t *r, unsigned int k){
    unsigned int index = 0;
    for(int i=3; i < n && index < k; i+=3){
        r[index].num   = converte_char_baralho(data[i]);
        r[index].naipe = converte_char_naipe(data[i+1]);
        index++;
    }
}

int unico_sobrevivente(uint8_t *vidas, uint8_t n){
    int num_vivos = 0;


    for(int i = 0; i < n; ++i){
        if(vidas[i] != 0)
            num_vivos++; 

        if(num_vivos >= 2)
            return 0;
    }

    return 1;
}

uint8_t calcula_vitoria(struct carta_t *r, uint8_t *v, unsigned int n, struct carta_t gato){
    unsigned int index_highest = 0;
    struct carta_t highest_card = r[0]; 

    if(highest_card.num == gato.num && highest_card.naipe == PAUS){
        v[index_highest]++;
        return index_highest;
    }

    for(int i=1; i < n; ++i){
        if(r[i].num != MORTO){

            //Verifica se a carta atual não é um gato de Paus (carta mais forte e imbatível do jogo)
            if(r[i].num == gato.num){
                if(r[i].naipe == PAUS){
                    v[i]++;
                    return i;
                }
            }

            //Verifica se a carta r não é maior que a maior carta atual (que não pode ser o gato)
            if((r[i].num > highest_card.num && highest_card.num != gato.num) || highest_card.num == MORTO){
                highest_card = r[i];
                index_highest = i;
            }
            else{
                //Verifica se as cartas não estão "empatadas" em seus valores
                if(r[i].num == highest_card.num){
                    //Se o naipe de r[i] for maior que o naipe de highest_cart r[i] passa a ser a maior carta 
                    if(r[i].naipe > highest_card.naipe){
                        highest_card = r[i];
                        index_highest = i;                 
                    }
                }
            }

        }
    }

    v[index_highest]++;

    return index_highest;
}

void descontar_vidas_perdidas(uint8_t *vidas, uint8_t *apostas, uint8_t *vitorias, uint8_t n){
    uint8_t desconto; 

    for(int i = 0; i < n; ++i){
        if(vidas[i] != 0){
                    //Para evitar dar numero negativo
            if(vitorias[i] > apostas[i])
                desconto = vitorias[i] - apostas[i];
            else 
                desconto = apostas[i] - vitorias[i];

            if(desconto > vidas[i])
                vidas[i] = 0;
            else 
                vidas[i] -= desconto;
        }
    }
}

int vida_final_partida(char *data, unsigned int n, int checkpointVidas, unsigned int index, unsigned int maxHand){
    int vidaFinalPartida;
    int indexData;
    
    for(int i = 0; i < n-1; i+=5){
       indexData = converte_char_int(data[i]);

       if(indexData == index){
            vidaFinalPartida = converte_char_int(data[i+3]);

            printf("\n|||||||||||||||||||||||||||||||||||||\n");
            printf("\nVIDAS PERDIDAS PARTIDA COM MAO %d: %d\n", maxHand, checkpointVidas - vidaFinalPartida);
            printf("\n|||||||||||||||||||||||||||||||||||||\n");

            return vidaFinalPartida;
       }
    }

    perror("ERRO: Indíce não encontrado\n");

    return -1;
}

void print_mao(struct carta_t *mao, unsigned int n, unsigned int round){
        
    printf("\n###### CARTAS NA MAO - RODADA %d ######\n", round);  
    printf("\n");
    print_deck(mao, n);
    printf("\n");
    printf("######################################\n");  

}

void print_apostas(char *apostas, unsigned int n , int maxHand){
    
    printf("\n--------- APOSTAS COM MAO %d --------\n", maxHand);  
    printf("\n");    
    for(int i = 0; i < n; i+=2){

        if(converte_char_int(apostas[i]) != MORTO)
            printf("JOGADOR %d diz que faz %d\n", i/2, converte_char_int(apostas[i]));
        else 
            printf("JOGADOR %d está MORTO\n", i/2);
    }
    printf("\n");
    printf("-------------------------------------\n");

}

void print_mesa(char *data, unsigned int n){

    printf("\n$$$$$$$$$$$$$$$$ MESA $$$$$$$$$$$$$$$\n");  
    printf("\n");
    printf("GATO: %c de %s\n", data[0], converte_char_naipe_string(data[1]));

    for(int i = 3; i < n-1; i+=3){

        if(converte_char_baralho(data[i]) != MORTO)
            printf("JOGADOR %d: %c de %s\n", (i-3)/3 , data[i], converte_char_naipe_string(data[i+1]));
        else 
            printf("JOGADOR %d MORTO\n", (i-3)/3);

    }
    printf("\n");
    printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");

}

void print_resultado_rodada(char *data, unsigned int n, int round){

    printf("\n******* RESULTADO RODADA %d *******\n", round);
    printf("\n");
    printf("GATO: %c de %s\n", data[0], converte_char_naipe_string(data[1]));
    printf("VENCEDOR: JOGADOR %d - %c de %s \n", converte_char_int(data[3]), data[4], converte_char_naipe_string(data[5]));
    for(int i = 7; i < n-1; i+=4){

        if(converte_char_int(data[i+1]) != MORTO)
            printf("JOGADOR %d: %c de %s\n", converte_char_int(data[i]) , data[i+1], converte_char_naipe_string(data[i+2]));
        else 
            printf("JOGADOR %d ESTÁ MORTO\n", converte_char_int(data[i]));
    }
    printf("\n");
    printf("*************************************\n");
}

void print_resultado_partida(char *data, unsigned int n, int maxHand){

    printf("\n!!!! RESULTADO PARTIDA COM MAO %d !!!!\n", maxHand);
    printf("\n");
    for(int i = 0; i < n-1; i+=5){

        if(converte_char_int(data[i+1]) != MORTO){
            printf("JOGADOR %d:\n", converte_char_int(data[i]));
            printf("    APOSTAS: Fazia %c\n", data[i+1]);
            printf("    FEZ: %c\n", data[i+2]);
            printf("    VIDAS AO FIM DA RODADA: %c\n", data[i+3]);
        }
        else 
            printf("JOGADOR %d ESTÁ MORTO\n", converte_char_int(data[i]));
       
    }
    printf("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
}

void print_fim_jogo(char *data, unsigned int n, int maxHand){
    int maior_vida = converte_char_int(data[1]); 
    int index_vencedor = 0; 
    int vida_i; 
    int empate = 0;

    printf("\n==== RESULTADO FINAL NA MAO %d ====\n", maxHand);
    printf("\n");
    printf("JOGADOR %d: Possui %d vidas\n", index_vencedor, maior_vida);
    for(int i=3; i < n-1; i+=3){

        vida_i = converte_char_int(data[i+1]);
        
        if(vida_i > maior_vida){
            maior_vida = vida_i;
            index_vencedor =  converte_char_int(data[i]);
            empate = 0;

        }
        else{
            if(vida_i == maior_vida){
                empate = 1;
            }
        }

        printf("JOGADOR %d: Possui %d vidas\n", converte_char_int(data[i]), vida_i);
    }

    if(empate)
        printf("EMPATE! NINGUÉM GANHOU! :(\n");
    else 
        printf("JOGADOR %d GANHOU COM %d VIDAS! *foguinhos*\n", index_vencedor, maior_vida);
    printf("\n");
}

int apostar(int round){
    int a, ok = 0; 

    printf("Faça sua aposta para a rodada com %d:", round);
    scanf("%d",&a);

    while(!ok){
        if(a <= round && a >= 0){
            ok = 1;
        }
        else{
            printf("ERRO: Sua aposta deve estar entre 1 e %d\nAposte novamente:", round);
            scanf("%d",&a);
        }
    }

    return a;
}

int escolhe_cartas(struct carta_t *v, int n){
    int c, ok = 0; 

    printf("Escolha sua carta:");
    scanf("%d",&c);

    while(!ok){
        if((c-1 >= 0 && c-1 < n) && v[c-1].num != USADA)
            ok = 1;
        else{
            printf("ERRO: Carta Inválida. OTÀRIO\nEscolha de novo:");
            scanf("%d",&c);
        }
    }

    return c-1;
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
        case 255:
            return 'X';
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
        case 255:
            return 'X';
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
        case 'X':
            return 255;
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
        case 'X':
            return 255;
            break;
    }

    return -1;
}

char *converte_char_naipe_string(char i){

    switch(i){
        case 'O':
            return "OUROS";
            break; 
        case 'E':
            return "ESPADAS";
            break; 
        case 'C':
            return "COPAS";
            break; 
        case 'P':
            return "PAUS";
            break;
        case 'X':
            return "MORTO";
            break;
    }

    return "UNKNOWN";
}

char *converte_int_naipe_string(int i){

    switch(i){
        case 0:
            return "OUROS";
            break; 
        case 1:
            return "ESPADAS";
            break; 
        case 2:
            return "COPAS";
            break; 
        case 3:
            return "PAUS";
            break;
        case 255:
            return "MORTO";
            break;
    }

    return "UNKNOWN";
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
        case 'X':
            return 255; 
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
        case 255:
            return 'X';
            break;
    }

    return -1;
}

void header_jogo_dane_se() {

    printf("⠀                           ⠘⡀⠀⠀⠀JOGO: DANE-SE⠀⠀⠀⠀⠀ ⡜⠀⠀⠀\n");
    printf("⠀⠀                          ⠀⠑⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⡔⠁⠀⠀⠀\n");
    printf("⠀⠀⠀⠀                          ⠈⠢⢄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⠴⠊⠀⠀⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀                         ⢸⠀⠀⠀⢀⣀⣀⣀⣀⣀⡀⠤⠄⠒⠈⠀⠀⠀⠀⠀⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀                         ⠘⣀⠄⠊⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⣤⣤⣤⣶⣤⣤⣀⣀⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣠⣴⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣶⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣾⣿⣿⣿⣿⣿⡿⠋⠉⠛⠛⠛⠿⣿⠿⠿⢿⣇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣾⣿⣿⣿⣿⣿⠟⠀⠀⠀⠀⠀⡀⢀⣽⣷⣆⡀⠙⣧⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢰⣿⣿⣿⣿⣿⣷⠶⠋⠀⠀⣠⣤⣤⣉⣉⣿⠙⣿⠀⢸⡆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⣿⣿⣿⣿⠁⠀⠀⠴⡟⣻⣿⣿⣿⣿⣿⣶⣿⣦⡀⣇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢨⠟⡿⠻⣿⠃⠀⠀⠀⠻⢿⣿⣿⣿⣿⣿⠏⢹⣿⣿⣿⢿⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⣼⣷⡶⣿⣄⠀⠀⠀⠀⠀⢉⣿⣿⣿⡿⠀⠸⣿⣿⡿⣷⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢻⡿⣦⢀⣿⣿⣄⡀⣀⣰⠾⠛⣻⣿⣿⣟⣲⡀⢸⡿⡟⠹⡆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢰⠞⣾⣿⡛⣿⣿⣿⣿⣰⣾⣿⣿⣿⣿⣿⣿⣿⣿⡇⢰⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠘⠀⣿⡽⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⢿⠿⣍⣿⣧⡏⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⣷⣿⣿⣿⣿⣿⣿⣿⣷⣮⣽⣿⣷⣙⣿⡟⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡟⣹⡿⠇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠛⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡧⣦⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⡆⠀⠀⠀⠀⠀⠀⠀⠉⠻⣿⣿⣾⣿⣿⣿⣿⣿⡶⠏⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⣀⣠⣤⡴⠞⠛⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠚⣿⣿⣿⠿⣿⣿⠿⠟⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n");
    printf("⠀⢀⣠⣤⠶⠚⠉⠉⠀⢀⡴⠂⠀⠀⠀⠀⠀⠀⠀⠀⢠⠀⠀⢀⣿⣿⠁⠀⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n");
    printf("⠞⠋⠁⠀⠀⠀⠀⣠⣴⡿⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⣾⠀⠀⣾⣿⠋⠀⢠⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n");
    printf("⡀⠀⠀⢀⣷⣶⣿⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⣆⣼⣿⠁⢠⠃⠈⠓⠦⣄⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n");
    printf("⣿⣿⡛⠛⠿⠿⠿⠿⠿⢷⣦⣤⣤⣤⣦⣄⣀⣀⠀⢀⣿⣿⠻⣿⣰⠻⠀⠸⣧⡀⠀⠉⠳⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n");
    printf("⠛⢿⣿⣆⠀⠀⠀⠀⠀⠀⠀⠀⠈⠉⠉⠙⠛⠿⣦⣼⡏⢻⣿⣿⠇⠀⠁⠀⠻⣿⠙⣶⣄⠈⠳⣄⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n");
    printf("⠀⠀⠈⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠁⣐⠀⠀⠀⠈⠳⡘⣿⡟⣀⡠⠿⠶⠒⠟⠓⠀⠹⡄⢴⣬⣍⣑⠢⢤⡀⠀⠀⠀⠀⠀⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⢀⣀⠐⠲⠤⠁⢘⣠⣿⣷⣦⠀⠀⠀⠀⠀⠀⠙⢿⣿⣏⠉⠉⠂⠉⠉⠓⠒⠦⣄⡀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠀⠀⠀⠀⠈⣿⣿⣷⣯⠀⠀⠀⠀⠀⠀⠀⠀⠉⠻⢦⣷⡀⠀⠀⠀⠀⠀⠀⠉⠲⣄⠀\n");
    printf("⠠⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠘⢦⠀⢹⣿⣏⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⢻⣷⣄⠀⠀⠀⠀⠀⠀⠈⠳\n");
    printf("⠀⠀⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠁⣸⣿⣿⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⣽⡟⢶⣄⠀⠀⠀⠀⠀\n");
    printf("⠯⠀⠀⠀⠒⠀⠀⠀⠀⠀⠐⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢻⣿⣿⣷⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⡄⠈⠳⠀⠀⠀⠀\n");
    printf("⠀⠀⢀⣀⣀⡀⣼⣤⡟⣬⣿⣷⣤⣀⣄⣀⡀⠀⠀⠀⠀⠀⠀⠈⣿⣿⡄⣉⡀⠀⠀⠀⠀⠀⠀⠀⢀⠀⠀⠀⠀⠀⣿⣿⣄⠀⣀⣀⡀⠀\n");
}