#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "lib_cards.h"


#define PORT_BASE 5000
#define BUF_SIZE 256
#define NUM_NODES 4
#define MAX_DATA_LENGHT 2048
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


int bind_socket(int sock, struct sockaddr_in *addr, unsigned int index){
    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = htonl(INADDR_ANY);
    addr->sin_port = htons(PORT_BASE + index);

    if(bind(sock, (struct sockaddr*)addr, sizeof(*addr)) == -1)
        return 0; 

    return 1;
}

void setar_proximo_nodo(struct sockaddr_in *node, unsigned int index){
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

void preparar_mensagem(struct message_frame *frame, char *data){
    frame->start = START;
    frame->size  = MAX_DATA_LENGHT+1;
    frame->flag  = SHUFFLE_FLAG;
    frame->round = 0;
    memcpy(frame->data, data, MAX_DATA_LENGHT+1);
}

int main(int argc, char *argv[]){
    int sock, index; 
    struct sockaddr_in my_addr, next_node_addr, from_addr;   
    
    struct message_frame message; 
    
    char token[TOKEN_SIZE+1];
    char buffer[TOKEN_SIZE+1];
    char data_buffer[MAX_DATA_LENGHT+1];
    char pack[FRAME_SIZE];
    unsigned int *baralho;
    struct carta_t *mao;


    memset(buffer, 0, TOKEN_SIZE+1);
    memset(buffer, 0, MAX_DATA_LENGHT+1);
    snprintf(token, TOKEN_SIZE+1,"3SAwABfnXZAPSr9zIjoWtA4rcJNRcZjSSYlLnBcSKwpthrOc9Tv7xNrIYrxzcqi6");


    index = atoi(argv[1]);
    int next_node_index = (index + 1) % NUM_NODES;
    printf("Index: %d\n", index);

    sock = socket(PF_INET, SOCK_DGRAM,0 );
    if(sock == -1)
        perror("Falha ao criar socket\n");

    if(!bind_socket(sock, &my_addr, index))
        perror("Falha no bind do socket\n");
    
    printf("Socket para %d: %d\n", index, sock);
    setar_proximo_nodo(&next_node_addr, next_node_index);

    socklen_t addr_size;
    //int opt; 
    int env; 

    if(index == 0){
        inicializa_frame(&message);
        strcpy(buffer, token);
        baralho = malloc(TAM_BARALHO*sizeof(unsigned int));
        mao = malloc(5*sizeof(struct carta_t));
        memset(baralho, 0, TAM_BARALHO);
        gera_cartas_aleatorias(mao, baralho, 5);
    }

    while(1){



        if(strcmp(buffer, token) == 0){
            printf("TOKEN: %s\n", buffer); 
            char pick[10];
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
            preparar_mensagem(&message, data_buffer);

            env =  sendto(sock, (char*)&message, FRAME_SIZE, 0, (struct sockaddr*)&next_node_addr, sizeof(next_node_addr));
            if(env < 0) 
                perror("Falha ao fazer sendto()\n");

            printf("Transmitindo de %d para %d\n", index, next_node_index);
            env =  sendto(sock, buffer, strlen(buffer) + 1, 0, (struct sockaddr*)&next_node_addr, sizeof(next_node_addr));
            if(env < 0) 
                perror("Falha ao fazer sendto()\n");
        }

        addr_size = sizeof(from_addr);
        env = recvfrom(sock, pack, FRAME_SIZE, 0, (struct sockaddr*)&from_addr, &addr_size);
        memcpy(&message, pack, FRAME_SIZE);
        if (env < 0)
            printf("Nothing received in %d\n", index);
        else 
            printf("Bytes lidos: %d\nMessage: %s\n", index, message.data);

        env = recvfrom(sock, buffer, TOKEN_SIZE+1, 0, (struct sockaddr*)&from_addr, &addr_size);
        if (env < 0)
            printf("Nothing received in %d\n", index);
        else 
            printf("Recebendo bastão em %d\nBytes lidos: %d\n", index,env);
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
