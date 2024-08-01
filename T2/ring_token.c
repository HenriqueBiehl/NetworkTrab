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
#define FRAME_SIZE 2058


#define START 0x7e
#define SHUFFLE_FLAG 0 
#define BET_FLAG 1
#define MATCH_FLAG 2 
#define RESULTS_FLAG 3
#define ALL_BETS 4

#define MAX_ROUNDS 9
#define TAM_BARALHO 40

struct message_frame{
    uint8_t start;                        //Bits de inicio transmissão
    unsigned int size;                    //Define o tamanho do campo *data
    uint8_t dest;                         //Define o índice de destino
    uint8_t flag;                         //Flags que definem o "momento do jogo" (embaralhamento, aposta, partida, resultados)
    uint8_t round;                        //Determina em qual rodada está o jogo 
    uint8_t num_cards;                    //Informa quantas cartas estão presentes na rodada
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
    frame->dest  = 0;
    frame->flag  = SHUFFLE_FLAG;
    frame->round = 0;
    frame->num_cards = 0;
    memset(frame->data, 0, MAX_DATA_LENGHT+1);
}

void preparar_mensagem(struct message_frame *frame, char *data, unsigned int size, int flag, int round, int num_cards, uint8_t dest){
    frame->start = START;
    frame->size  = size;
    frame->dest = dest;
    frame->flag  = flag;
    frame->round = round;
    frame->num_cards = num_cards;
    memset(frame->data, 0, MAX_DATA_LENGHT+1);
    memcpy(frame->data, data, size+1);
}


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


struct token_ring incializa_token(){
    struct token_ring t;

    t.start = START;
    memset(t.token, 0, TOKEN_SIZE+1);

    return t;
}

struct message_frame seta_mensagem_shuffle(unsigned int *baralho, int dest_index, uint8_t round, int n){
    struct message_frame cards; //Frame com as cartas a serem enviadas

    struct carta_t *deck; //Vetor que representará a 'mão' de cartas 

    deck = malloc(n*sizeof(struct carta_t)); //Aloca o "deck" que será enviado 
    gera_cartas_aleatorias(deck, baralho, n); //Gera as cartas do deck marcando como usada no baralho

    /* Inicializa o frame com os dados necessários */
    cards.start = START;
    cards.flag  = SHUFFLE_FLAG;
    cards.dest  = dest_index;
    cards.round = round;
    memset(cards.data, 0, MAX_DATA_LENGHT+1);

    /* Converte o vetor deck para uma string formatada em cards.data. Recebe o ponteo de cards.size para salvar o tamanho*/
    mao_baralho(deck, n, &cards.size, cards.data);
    cards.num_cards = n; 
    cards.size++; //Adiciona o tamanho do '\0'

    /*Libera o vetor do deck q nn será mais útil*/
    free(deck);

    return cards;
}

struct carta_t *vetor_cartas(char *data, unsigned int n, uint8_t num_cards){
    struct carta_t *v; 
    unsigned int index = 0;
    
    printf("Convertendo deck\n");

    v = malloc(num_cards*sizeof(struct carta_t));
    
    for(int i = 0; i < n; i+=3){
        printf("i:%c e i+1:%c\n", data[i], data[i+1]);
        v[index].num = converte_char_baralho(data[i]); 
        v[index].naipe = converte_char_naipe(data[i+1]);
        index++;
    }

    return v;
} 

void converte_apostas(char *data, unsigned int n, uint8_t *a, unsigned int k){
    unsigned int index = 0;

    for(int i=0; i < n && index < k; i+=2){
        a[index] = converte_char_int(data[i]);
        index++;
    }
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
        if((c-1 > 0 && c-1 < n) && v[c].num != USADA)
            ok = 1;
        else{
            printf("ERRO: Carta Inválida. OTÀRIO\n");
            scanf("%d",&c);
        }
    }

    return c;
}


void receber_token(int sock, struct token_ring *node_token ,struct sockaddr_in from_addr, socklen_t addr_size){
    char token_buffer[TOKEN_RING_SIZE+1];

    printf("Recebendo o token\n");
    if (recvfrom(sock, token_buffer, TOKEN_RING_SIZE, 0, (struct sockaddr*)&from_addr, &addr_size) < 0)
        perror("No token received\n");            
    memcpy(node_token, token_buffer, TOKEN_RING_SIZE);
}


int main(int argc, char *argv[]){
    int sock, index; 
    struct sockaddr_in my_addr, next_node_addr, from_addr;   
    uint8_t MASTER_FLAG, dest;
    uint8_t *vidas; 
    uint8_t *apostas;
    uint8_t *vitorias;


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
    struct carta_t *deck;

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

    int opt; 

    printf("Iniciar:\n[1] - Sim\n[2]-Sair\n");
    scanf("%d", &opt);
    if(opt == 2){
        close(sock);
        return 1;
    }


    if(index == 0){
        MASTER_FLAG = SHUFFLE_FLAG;    //Flag que só o indice 0 (o mestre do jogo) possui indicando o que ele deve fazer na rede; 
        dest = 0;
        strcpy(node_token.token, token);

        vidas = malloc(sizeof(uint8_t)*NUM_NODES);
        memset(vidas, 3, NUM_NODES);

        apostas = malloc(sizeof(uint8_t)*NUM_NODES);
        memset(apostas, 0, NUM_NODES);

        vitorias = malloc(sizeof(uint8_t)*NUM_NODES);
        memset(apostas, 0, NUM_NODES);


        baralho = malloc(sizeof(unsigned int)*TAM_BARALHO);
        memset(baralho, 0, TAM_BARALHO*sizeof(unsigned int));
    }

    int aposta, round = 1; 
    dest = 0;
    int n = 1;
    struct carta_t carta;
    while(1){

        /* Sequência de operações para quando se tem o token*/
        if(node_token.start == START){

            if(strcmp(node_token.token, token) == 0){
                printf("TOKEN: %s\n", node_token.token); 
                
                /*Comportamentos de envio específicos para o nodo 0*/
                if(index == 0){

                    /* Nodo 0 embaralha as cartas*/
                    switch(MASTER_FLAG){

                        case SHUFFLE_FLAG:
                            {
                                if(dest == 0){
                                    deck = malloc(n*sizeof(struct carta_t));
                                    gera_cartas_aleatorias(deck, baralho, n);
                                }
                                dest++;
                                
                                if(dest < NUM_NODES)
                                    message = seta_mensagem_shuffle(baralho, dest, round, n);
                                
                                printf("Preparado \n");

                            }
                            break;
                        
                        case BET_FLAG:
                            {
                                aposta = apostar(round);
                                printf("Apostado %d\n", aposta);
                                memset(data_buffer, 0, MAX_DATA_LENGHT+1);
                                snprintf(data_buffer, 3 ,"%c|", converte_int_char(aposta));
                                preparar_mensagem(&message, data_buffer, strlen(data_buffer)+1, BET_FLAG, round, 0, n);
                            }
                            break; 
                        
                        case ALL_BETS:
                            {
                                printf("    All bets case\n");
                                message.flag = ALL_BETS; 
                                message.dest = next_node_index; 
                            }
                            break;
                
                        case MATCH_FLAG:
                            {
                                struct carta_t gato; 

                                gato = carta_aleatoria(baralho);

                                printf("*** HAND CARDS ***\n");
                                print_deck(deck, n);
                                printf("******************\n");

                                printf("Escolha a carta:\n");
                                opt = escolhe_cartas(deck, n);
                                cart = v[opt]; 
                                memset(data_buffer, 0, MAX_DATA_LENGHT+1);
                                snprintf(data_buffer, 4 ,"%c%c@%c%c|", converte_int_char(gato.num), converte_int_char(gato.naipe), converte_int_char(carta.num),converte_int_char(carta.naipe));
                                preparar_mensagem(&message, data_buffer, strlen(data_buffer)+1, MATCH_FLAG, round, n, 1);
                                
                            }
                            break; 

                        case RESULTS_FLAG:
                            break;

                    }                
                }
                else{
                    switch(message.flag){

                        case SHUFFLE_FLAG:
                            {
                                if(message.dest == index){
                                    memset(data_buffer, 0, MAX_DATA_LENGHT+1);
                                    snprintf(data_buffer, 12 ,"RECEBIDO:%c\n", converte_int_char(index));
                                    preparar_mensagem(&message, data_buffer, strlen(data_buffer)+1, SHUFFLE_FLAG, round, 0, 1);
                                }
                                printf("Preparado \n");
                            }
                            break;
                        
                        case BET_FLAG:
                            {
                                char bet[3];
                                aposta = apostar(round);
                                printf("Apostado %d \n", aposta);
                                memcpy(data_buffer, message.data, MAX_DATA_LENGHT+1);
                                snprintf(bet, 3 ,"%c|", converte_int_char(aposta));
                                strcat(data_buffer, bet);
                                preparar_mensagem(&message, data_buffer, strlen(data_buffer)+1, BET_FLAG, round, 0, next_node_index);
                            }
                            break; 

                        case ALL_BETS:
                            {
                                message.dest = next_node_index;
                            }
                            break; 
                        
                        case MATCH_FLAG: 
                            {
                                char play[4];
                                printf("*** HAND CARDS ***\n");
                                print_deck(deck, n);
                                printf("******************\n");

                                printf("Escolha a carta:\n");
                                opt = escolhe_cartas(deck, n);
                                cart = v[opt]; 
                                memcpy(data_buffer, message.data, MAX_DATA_LENGHT+1);
                                snprintf(play, 4 ,"%c%c|", converte_int_char(carta.num),converte_int_char(carta.naipe));
                                strcat(data_buffer, play);
                                preparar_mensagem(&message, data_buffer, strlen(data_buffer)+1, MATCH_FLAG, round, n, next_node_index);
                            }
                            break; 

                        case RESULTS_FLAG:
                            {
                                /*
                                    3   - Monta a mensagem com os dados de resultado contendo:
                                    3.1 - Resultado final da rodada
                                    3.2 - Vencendor da rodada
                                */  

                            }

                            break;

                    }
                }

                /* Adicionar casos especiais de envio*/
                    /* Exemplo, o jogador que morre recebe os dados da partida mas é simplesmente passa o bastão pra frente*/
                
                /* Envio da mensagem para o próximo nodo */
                printf("Irei enviar %s\n", message.data);
                if(sendto(sock, (char*)&message, FRAME_SIZE, 0, (struct sockaddr*)&next_node_addr, sizeof(next_node_addr))< 0) 
                    perror("Falha ao fazer sendto()\n");
                else    
                    printf("%s enviado\n", message.data);
                
                /* Envio do token para o próximo nodo*/
                if(sendto(sock, (char*)&node_token, TOKEN_RING_SIZE, 0, (struct sockaddr*)&next_node_addr, sizeof(next_node_addr)) < 0) 
                    perror("Falha ao fazer sendto()\n");
                else    
                    printf("Token enviado\n");
            }
        
        }

        /* Recebendo uma nova mensagem e copiando para o frame de mensagem "message"*/
        memset(&message, 0 , FRAME_SIZE);
        addr_size = sizeof(from_addr);
        if (recvfrom(sock, frame_buffer, FRAME_SIZE, 0, (struct sockaddr*)&from_addr, &addr_size) < 0)
            printf("Nothing received in %d\n", index);
        else    
            printf("Received %s\n", message.data);
        memcpy(&message, frame_buffer , FRAME_SIZE);
        
        /* Lendo a mensagem para operar nela*/
        if(message.start == START){
            printf("INCIANDO MENSAGEM\nBytes lidos: %d\nMessage: %s\n", index, message.data);

            
            if(index == 0){
                /* Comportamentos específicos para o indice 0 */
                /* Checar o fim de rodada e passar os resultados da rodada */
            }

            /* Opera de acordo com a flag do jogo */
            /* Se atentar ao fato que não é sempre que se recebe o token, em shuffle você só escuta*/
            switch(message.flag){

                case SHUFFLE_FLAG: 
                    {                
                        if(message.dest == 0 && index == 0){
                            printf("%s\n", message.data);

                            if(dest == 3){
                                MASTER_FLAG = BET_FLAG; 
                                dest = 0;   
                            }
                        }
                        else if (message.dest == index){
                            deck = vetor_cartas(message.data, message.size, message.num_cards);
                            print_deck(deck, message.num_cards);
                        } 
                    }
                    
                    break; 

                case BET_FLAG:
                    {
                        printf("Apostas da partida: %s\n", message.data);
                        
                        if(message.dest == 0 && index == 0){
                            converte_apostas(message.data, message.size, apostas, 4);

                            for(int i=0; i < 4; ++i)
                                printf("a[%d] = %d\n", i, apostas[i]);

                            MASTER_FLAG = ALL_BETS;
                        }
                    }
                    break; 

                case ALL_BETS:
                    {
                        printf("Todas as Apostas:\n%s\n", message.data);

                        if(message.dest == 0 && index == 0){
                            MASTER_FLAG = MATCH_FLAG;
                        }
                    }
                    break; 

                case MATCH_FLAG:
                    {
                        printf("*** MATCH ***\n");
                        printf("%s\n", message.data);

                        /*
                            Verificar se o destino é 0. 
                            Se for:
                                1 - Computa o vetor e o resultado (incluindo salvar o vencedor da rodada)
                                    1.1 - Utilizar do vetor vitorias
                                2 - Muda a flag de MASTER para RESULTS_FLAG
                        
                        */


                    }

                    break; 

                case RESULTS_FLAG:{
                        /*
                            Imprimir o Resultado
                            1. Voltar para o estado de MATCH
                            2. Computar os resultados finais
                            2. Se ele finalizou a mao, voltar para o estado de SHUFFLE (usar variável ROUND_MAX) 
                        */
                    }
                    break;
                
                default:
                    break;
            }

        }        

        receber_token(sock, &node_token , from_addr, addr_size);

    }
       
    close(sock);
}