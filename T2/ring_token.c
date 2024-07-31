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
#define FRAME_SIZE 2055


#define START 0x7e
#define SHUFFLE_FLAG 0 
#define BET_FLAG 1
#define MATCH_FLAG 2 
#define RESULTS_FLAG 3

#define MAX_ROUNDS 9
#define TAM_BARALHO 40

struct message_frame{
    uint8_t start;                        //Bits de inicio transmissão
    unsigned int size;                    //Define o tamanho do campo *data
    uint8_t flag;                         //Flags que definem o "momento do jogo" (embaralhamento, aposta, partida, resultados)
    uint8_t round;                        //Determina em qual rodada está o jogo 
    char data[MAX_DATA_LENGHT+1];         //Campo de dados onde são enviados os dados da partida 
};

struct token_ring{
    uint8_t start;                        //Bits de inicio transmissão
    char token[TOKEN_SIZE+1];
};

int bind_socket(int sock, struct sockaddr_in *addr, unsigned int index){
    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = htonl(INADDR_ANY);
    addr->sin_port = htons(PORT_BASE + index);

    if(bind(sock, (struct sockaddr*)addr, sizeof(*addr)) == -1)
        return 0; 

    return 1;
}

void setar_nodo(struct sockaddr_in *node, unsigned int index){
    memset(node, 0, sizeof(*node));
    node->sin_family = AF_INET;
    node->sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // Para simplicidade, usando loopback
    node->sin_port = htons(PORT_BASE + index);
}

void inicializa_frame(struct message_frame *frame){
    frame->start = START;
    frame->size  = MAX_DATA_LENGHT+1;
    frame->flag  = SHUFFLE_FLAG;
    frame->round = 0;
    memset(frame->data, 0, MAX_DATA_LENGHT+1);
}

void preparar_mensagem(struct message_frame *frame, char *data, unsigned int size, int flag, int round){
    frame->start = START;
    frame->size  = size;
    frame->flag  = SHUFFLE_FLAG;
    frame->round = 0;
    memset(frame->data, 0, MAX_DATA_LENGHT+1);
    memcpy(frame->data, data, size+1);
}


void mao_baralho(struct carta_t *c, int n, unsigned int *size, char *str){
    int msg_index = 0;
    struct carta_t x; 

    for(int i =0; i < n; ++i){
        x = c[i];
        str[msg_index] = converte_numero_baralho(x.num); 
        str[msg_index+1] = '/';
        str[msg_index+2] = converte_numero_naipe(x.naipe);
        str[msg_index+3] = '|';
        msg_index += 4;
    }

    *size = msg_index;
    str[msg_index] = '\0';
}


struct token_ring incializa_token(){
    struct token_ring t;

    t.start = START;
    memset(t.token, 0, TOKEN_SIZE+1);

    return t;
}

int enviar_cartas(unsigned int *baralho, int dest_index, int sock){
    struct sockaddr_in to_addr;
    struct message_frame cards; 
    int env;

    setar_nodo(&to_addr, dest_index);

    struct carta_t *mao;

    mao = malloc(5*sizeof(struct carta_t));
    gera_cartas_aleatorias(mao, baralho, 5);


    cards.start = START;
    cards.flag  = SHUFFLE_FLAG;
    cards.round = 0;
    memset(cards.data, 0, MAX_DATA_LENGHT+1);

    mao_baralho(mao, 5, &cards.size, cards.data);
    printf("%s\n", cards.data);
    cards.size++;
    
    env =  sendto(sock, (char*)&cards, FRAME_SIZE, 0, (struct sockaddr*)&to_addr, sizeof(to_addr));
    if(env < 0) 
        perror("Falha ao fazer sendto()\n");

    return 1;
}

int main(int argc, char *argv[]){
    int sock, index; 
    struct sockaddr_in my_addr, next_node_addr, from_addr;   
    
    struct message_frame message; 
    char data_buffer[MAX_DATA_LENGHT+1];
    char frame_buffer[FRAME_SIZE];
    memset(data_buffer, 0, MAX_DATA_LENGHT+1);

    struct token_ring node_token = incializa_token();
    char token[TOKEN_SIZE+1];
    char token_buffer[TOKEN_RING_SIZE+1];

    snprintf(token, TOKEN_SIZE+1,"3SAwABfnXZAPSr9zIjoWtA4rcJNRcZjSSYlLnBcSKwpthrOc9Tv7xNrIYrxzcqi6");
    memset(token_buffer, 0, TOKEN_SIZE+1);

    unsigned int *baralho;
    struct carta_t *mao;

    index = atoi(argv[1]);
    int next_node_index = (index + 1) % NUM_NODES;
    printf("Index: %d\n", index);

    sock = socket(PF_INET, SOCK_DGRAM,0 );
    if(sock == -1)
        perror("Falha ao criar socket\n");

    if(!bind_socket(sock, &my_addr, index))
        perror("Falha no bind do socket\n");
    
    printf("Socket para %d: %d\n", index, sock);
    setar_nodo(&next_node_addr, next_node_index);

    socklen_t addr_size;
    int env; 

    if(index == 0){
        inicializa_frame(&message);
        strcpy(node_token.token, token);
        baralho = malloc(sizeof(unsigned int)*TAM_BARALHO);
        mao = malloc(5*sizeof(struct carta_t));
        memset(baralho, 0, TAM_BARALHO*sizeof(unsigned int));
        gera_cartas_aleatorias(mao, baralho, 5);
    }

    while(1){
        if(node_token.start == START){
            if(strcmp(node_token.token, token) == 0){
                printf("TOKEN: %s\n", node_token.token); 
                if(index == 0){
                    for(int i=0; i < NUM_NODES; ++i)
                        enviar_cartas(baralho, i, sock);                
                }
                /*char pick[10];
                memset(pick, 0, 10);
                char card;
                char house;  

                printf("Escolha uma casa e um número:\n");
                scanf(" %c",&card);
                getchar(); 
                scanf(" %c",&house);
                getchar(); 
                printf("Done\n");
                snprintf(pick, 10,"%d:%c/%c|",index,card,house);
                strcpy(data_buffer, strcat(message.data,pick));
                preparar_mensagem(&message, data_buffer, strlen(data_buffer)+1, 0,0);*/

                env =  sendto(sock, (char*)&message, FRAME_SIZE, 0, (struct sockaddr*)&next_node_addr, sizeof(next_node_addr));
                if(env < 0) 
                    perror("Falha ao fazer sendto()\n");

                printf("Transmitindo de %d para %d\n", index, next_node_index);
                env =  sendto(sock, (char*)&node_token, TOKEN_RING_SIZE, 0, (struct sockaddr*)&next_node_addr, sizeof(next_node_addr));
                if(env < 0) 
                    perror("Falha ao fazer sendto()\n");
            }
        }
        
        addr_size = sizeof(from_addr);
        env = recvfrom(sock, frame_buffer, FRAME_SIZE, 0, (struct sockaddr*)&from_addr, &addr_size);
        if (env < 0)
            printf("Nothing received in %d\n", index);
        
        memcpy(&message, frame_buffer , FRAME_SIZE);
        if(message.start == START){
            printf("Bytes lidos: %d\nMessage: %s\n", index, message.data);
        }

        env = recvfrom(sock, token_buffer, TOKEN_RING_SIZE, 0, (struct sockaddr*)&from_addr, &addr_size);
        if (env < 0)
            printf("Nothing received in %d\n", index);
        memcpy(&node_token, token_buffer, TOKEN_RING_SIZE);
    }

    close(sock);
}

  // printf("Escolha a opção:\n[1] - Finalizar \n[2] - Segredo \n[3] - :D\n");
            // scanf("%d", &opt);
        
            // if(opt == 1)
            //     break; 
        
            // if(opt == 2){
            //     printf("Make his fight on the hill in the early day\n");
            // }   

            // if(opt == 3){
            //     printf("Constant chills deep inside *DRUM SOUND*\n");
            // }
