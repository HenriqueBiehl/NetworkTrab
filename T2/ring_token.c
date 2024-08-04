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
#define ALL_BETS_FLAG 4
#define MATCH_RESULTS_FLAG 5
#define END_GAME_FLAG 255


#define LOCAL_PORT 12346                  // Porta local de recepção

#define MAX_CARTAS_MAO 9
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
    addr->sin_port = htons(PORT_BASE+index);

    if(bind(sock, (struct sockaddr*)addr, sizeof(*addr)) == -1)
        return 0; 

    return 1;
}

void setar_nodo_mult_maquinas(struct sockaddr_in *node, unsigned short port){
    memset(node, 0, sizeof(*node));
    node->sin_family = AF_INET;
    node->sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // Para simplicidade, usando loopback
    node->sin_port = htons(port);
}

void setar_nodo_loop_back(struct sockaddr_in *node, unsigned int index){
    memset(node, 0, sizeof(*node));
    node->sin_family = AF_INET;
    node->sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // Para simplicidade, usando loopback
    node->sin_port = htons(PORT_BASE +index);
}

struct token_ring incializa_token(){
    struct token_ring t;

    t.start = START;
    memset(t.token, 0, TOKEN_SIZE+1);

    return t;
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

void gera_mensagem_resultado(char *data, unsigned int *tam, struct carta_t *r, uint8_t n, uint8_t ganhador, struct carta_t gato){
    char buff[5];
    uint8_t nm; 
    uint8_t np;

    memset(buff, 0, 5);
    snprintf(buff,5, "%c%c@", converte_numero_baralho(gato.num), converte_numero_naipe(gato.naipe));
    strcat(data, buff);
    
    nm = r[ganhador].num; 
    np = r[ganhador].naipe; 
    memset(buff, 0, 5);
    snprintf(buff, 5,"%c%c%c|", converte_int_char(ganhador), converte_numero_baralho(nm), converte_numero_naipe(np));
    strcat(data, buff);

    for(int i=0; i < n; ++i){
        memset(buff, 0, 5);
        nm = r[i].num; 
        np = r[i].naipe; 
        snprintf(buff, 5 ,"%c%c%c|",converte_int_char(i), converte_numero_baralho(nm), converte_numero_naipe(np));
        strcat(data, buff);
    }
    *tam = strlen(data); 
}

void gera_mensagem_partida(char *data, unsigned int *tam, uint8_t *apostas, uint8_t *vitorias, uint8_t *vidas , uint8_t n){
    char buff[6];

    for(int i=0; i < n; ++i){
        memset(buff, 0, 6);
        snprintf(buff, 6 ,"%c%c%c%c|",converte_int_char(i), converte_int_char(apostas[i]), converte_int_char(vitorias[i]), converte_int_char(vidas[i]));
        strcat(data, buff);
    }
    *tam = strlen(data); 
}

void gerar_mensagem_fim_jogo(char *data, unsigned int *tam, uint8_t *vidas, unsigned int n){
    char buff[4];
    
    for(int i=0; i < n; ++i){
        memset(buff, 0, 4);
        snprintf(buff, 4 ,"%c%c|",converte_int_char(i), converte_int_char(vidas[i]));
        strcat(data, buff);
    }

    *tam = strlen(data);
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


void receber_token(int sock, struct token_ring *node_token ,struct sockaddr_in from_addr, socklen_t addr_size){
    char token_buffer[TOKEN_RING_SIZE+1];

    //printf("Recebendo o token\n");
    if (recvfrom(sock, token_buffer, TOKEN_RING_SIZE, 0, (struct sockaddr*)&from_addr, &addr_size) < 0)
        perror("No token received\n");            
    memcpy(node_token, token_buffer, TOKEN_RING_SIZE);
}


int main(int argc, char *argv[]){
    header_jogo_dane_se();

    int sock, index; 
    struct sockaddr_in my_addr, next_node_addr, from_addr;   
    uint8_t MASTER_FLAG, dest;
    uint8_t *vidas; 
    uint8_t *apostas;
    uint8_t *vitorias;
    struct carta_t *rodada;
    unsigned short port = PORT_BASE; 

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

    if(argc == 4){
        printf("** Uso com múltiplas maquinas **");
        port = atoi(argv[3]);
        // Converter IP de string para endereço binário
        setar_nodo_mult_maquinas(&next_node_addr, port);
        if (inet_pton(AF_INET, argv[2], &next_node_addr.sin_addr) <= 0) {
            perror("inet_pton");
            exit(1);
        }
    }
    else {
        port += next_node_index;
        setar_nodo_loop_back(&next_node_addr, next_node_index);
    }

    sock = socket(PF_INET, SOCK_DGRAM,0 );
    if(sock == -1)
        perror("Falha ao criar socket\n");

    if(!bind_socket(sock, &my_addr, index))
        perror("Falha no bind do socket\n");
    
    //printf("Socket para %d: %d\n", index, sock);

    socklen_t addr_size;

    int opt; 

    printf("\n@@@@@@@@@@@@ INICIAR JOGO @@@@@@@@@@@@\n");
    printf("\n[1]- Sim\n[2]- Sair\nEscolha:");
    scanf("%d", &opt);
    printf("\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
    if(opt == 2){
        close(sock);
        return 1;
    }

    if(index == 0){
        MASTER_FLAG = SHUFFLE_FLAG;    //Flag que só o indice 0 (o mestre do jogo) possui indicando o que ele deve fazer na rede; 
        dest = 0;
        strcpy(node_token.token, token);
        memset(&message, 0 , FRAME_SIZE);


        vidas = malloc(sizeof(uint8_t)*NUM_NODES);
        memset(vidas, 4, NUM_NODES);

        apostas = malloc(sizeof(uint8_t)*NUM_NODES);
        memset(apostas, 0, NUM_NODES);

        vitorias = malloc(sizeof(uint8_t)*NUM_NODES);
        memset(vitorias, 0, NUM_NODES);
        
        rodada = malloc(sizeof(struct carta_t)*NUM_NODES);
        memset(rodada, 0, NUM_NODES);
        
        baralho = malloc(sizeof(unsigned int)*TAM_BARALHO);
        memset(baralho, 0, TAM_BARALHO*sizeof(unsigned int));


    }

    struct carta_t carta, gato; 
    int cartas_mao = 1; 
    int round = 1;
    int aposta;     
    int ganhador;
    uint8_t contVidas = 4;

    while(1){

        /* Sequência de operações para quando se tem o token*/
        if(node_token.start == START){

            if(strcmp(node_token.token, token) == 0){
                //printf("TOKEN: %s\n", node_token.token); 
                
                /*Comportamentos de envio específicos para o nodo 0*/
                if(index == 0){

                    /* Nodo 0 embaralha as cartas*/
                    switch(MASTER_FLAG){

                        case SHUFFLE_FLAG:
                            {
                                if(dest == 0){
                                    gato = carta_aleatoria(baralho);
                                    if(contVidas != 0){
                                            deck = malloc(cartas_mao*sizeof(struct carta_t));
                                            gera_cartas_aleatorias(deck, baralho, cartas_mao);
                                            print_mao(deck, cartas_mao, round);
                                    }
                                }
                                dest++;
                                
                                if(dest < NUM_NODES){

                                    for(int i=dest; i < NUM_NODES; ++i){
                                            if(vidas[dest] != 0){
                                                struct carta_t *temp_deck;
                                                temp_deck = malloc(cartas_mao*sizeof(struct carta_t)); //Aloca o "deck" que será enviado 
                                                gera_cartas_aleatorias(temp_deck, baralho, cartas_mao); //Gera as cartas do deck marcando como usada no baralho
                                                memset(data_buffer, 0, MAX_DATA_LENGHT+1);
                                                mao_baralho(temp_deck, cartas_mao, &message.size, data_buffer);
                                                preparar_mensagem(&message, data_buffer, strlen(data_buffer)+1, SHUFFLE_FLAG , round, cartas_mao, dest);
                                                free(temp_deck);
                                                break;
                                            }
                                            else
                                                dest++;
                                    }
                                        

                                }                                
                            }
                            break;
                        
                        case BET_FLAG:
                            {
                                memset(data_buffer, 0, MAX_DATA_LENGHT+1);
                                if(contVidas != 0){
                                    printf("\n****** FAZER APOSTA PARTIDA %d ******\n", cartas_mao);
                                    aposta = apostar(cartas_mao);
                                    printf("Apostado %d\n", aposta);
                                    snprintf(data_buffer, 3 ,"%c|", converte_int_char(aposta));
                                    printf("*************************************\n");
                                }
                                else{
                                    snprintf(data_buffer, 3 ,"X|");
                                }
                                preparar_mensagem(&message, data_buffer, strlen(data_buffer)+1, BET_FLAG, round, cartas_mao, 1);

                            }
                            break; 
                        
                        case ALL_BETS_FLAG:
                            {
                                message.flag = ALL_BETS_FLAG; 
                                message.dest = next_node_index; 
                            }
                            break;
                
                        case MATCH_FLAG:
                            {
                                memset(data_buffer, 0, MAX_DATA_LENGHT+1);
                                snprintf(data_buffer, 4,"%c%c|", converte_numero_baralho(gato.num), converte_numero_naipe(gato.naipe));
                                print_mesa(data_buffer,3);
                                
                                memset(data_buffer, 0, MAX_DATA_LENGHT+1);
                                if(contVidas != 0){
                                    print_mao(deck, message.num_cards, round);

                                    printf("Escolha uma carta para jogar\n");
                                    opt = escolhe_cartas(deck, message.num_cards);
                                    carta = deck[opt]; 
                                    snprintf(data_buffer, 7 ,"%c%c@%c%c|", converte_numero_baralho(gato.num), converte_numero_naipe(gato.naipe), converte_numero_baralho(carta.num),converte_numero_naipe(carta.naipe));
                                    marcar_carta_usada(&deck[opt]);
                                }
                                else{
                                    snprintf(data_buffer, 7, "%c%c@XX|", converte_numero_baralho(gato.num), converte_numero_naipe(gato.naipe));
                                }
                                preparar_mensagem(&message, data_buffer, strlen(data_buffer)+1, MATCH_FLAG, round, cartas_mao, 1);
                                
                            }
                            break; 

                        case RESULTS_FLAG:
                            {          
                                memset(data_buffer, 0, MAX_DATA_LENGHT+1);
                                gera_mensagem_resultado(data_buffer, &message.size, rodada, 4, ganhador, gato);
                                preparar_mensagem(&message, data_buffer, strlen(data_buffer)+1, RESULTS_FLAG, round, cartas_mao, 1);   
                            }
                            break;
                        case MATCH_RESULTS_FLAG:
                            {
                                memset(data_buffer, 0, MAX_DATA_LENGHT + 1);
                                gera_mensagem_partida(data_buffer, &message.size, apostas, vitorias, vidas, 4);
                                preparar_mensagem(&message, data_buffer, strlen(data_buffer)+1, MATCH_RESULTS_FLAG, round, cartas_mao, 1); //Na hora de preparar a mensagem, se atentar que round e n estão com 1 a mais (uma checagem ternaria já ajuda)
                            }
                            break; 
                        case END_GAME_FLAG: 
                            {
                                memset(data_buffer, 0, MAX_DATA_LENGHT + 1);
                                gerar_mensagem_fim_jogo(data_buffer, &message.size, vidas, 4);
                                preparar_mensagem(&message, data_buffer, strlen(data_buffer)+1, END_GAME_FLAG, round, cartas_mao, 1); //Na hora de preparar a mensagem, se atentar que round e n estão com 1 a mais (uma checagem ternaria já ajuda)
                            }
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
                                    preparar_mensagem(&message, data_buffer, strlen(data_buffer)+1, SHUFFLE_FLAG, round, cartas_mao, 0);
                                }
                            }
                            break;
                        
                        case BET_FLAG:
                            {
                                char bet[3];
                                memcpy(data_buffer, message.data, MAX_DATA_LENGHT+1);
                                
                                if(contVidas != 0){
                                    printf("\n****** FAZER APOSTA PARTIDA %d ******\n", cartas_mao);
                                    aposta = apostar(cartas_mao);
                                    printf("Apostado %d \n", aposta);
                                    snprintf(bet, 3 ,"%c|", converte_int_char(aposta));  
                                    printf("*************************************\n");
                                }
                                else{
                                    snprintf(bet, 3 ,"X|");
                                }
                                strcat(data_buffer, bet);
                                preparar_mensagem(&message, data_buffer, strlen(data_buffer)+1, BET_FLAG, round, cartas_mao, next_node_index);

                            }
                            break; 

                        case ALL_BETS_FLAG:
                            {
                                message.dest = next_node_index;
                            }
                            break; 
                        
                        case MATCH_FLAG: 
                            {
                                char play[4];
                                memcpy(data_buffer, message.data, MAX_DATA_LENGHT+1);
                                
                                if(contVidas != 0){
                                    print_mao(deck, message.num_cards, round);
                                    printf("Escolha uma carta para jogar\n");
                                    opt = escolhe_cartas(deck, cartas_mao);
                                    carta = deck[opt]; 
                                    snprintf(play, 4 ,"%c%c|", converte_numero_baralho(carta.num),converte_numero_naipe(carta.naipe));
                                    marcar_carta_usada(&deck[opt]);
                                    
                                }
                                else{
                                    snprintf(play, 4, "XX|");
                                }
                                strcat(data_buffer, play);
                                preparar_mensagem(&message, data_buffer, strlen(data_buffer)+1, MATCH_FLAG, round, cartas_mao, next_node_index);
                            }
                            break; 

                        case RESULTS_FLAG:
                            {
                                message.dest = next_node_index;
                            }
                            break;
                        case MATCH_RESULTS_FLAG:
                            {
                                message.dest = next_node_index;
                            }
                        case END_GAME_FLAG:
                            {
                                message.dest = next_node_index;
                            }

                    }
                }
                
                /* Adicionar casos especiais de envio*/
                    /* Exemplo, o jogador que morre recebe os dados da partida mas é simplesmente passa o bastão pra frente*/
                
                /* Envio da mensagem para o próximo nodo */
                //printf("Irei enviar %s\n", message.data);
                if(sendto(sock, (char*)&message, FRAME_SIZE, 0, (struct sockaddr*)&next_node_addr, sizeof(next_node_addr))< 0) 
                    perror("Falha ao fazer sendto()\n");
                /*else    
                    printf("%s enviado\n", message.data);*/
                
                /* Envio do token para o próximo nodo*/
                if(sendto(sock, (char*)&node_token, TOKEN_RING_SIZE, 0, (struct sockaddr*)&next_node_addr, sizeof(next_node_addr)) < 0) 
                    perror("Falha ao fazer sendto()\n");
                /*else    
                    printf("Token enviado\n");*/

                if(message.flag == END_GAME_FLAG && index != 0){
                    break; //Encerra o loop de execução
                }
                
            }

        
        }

        /* Recebendo uma nova mensagem e copiando para o frame de mensagem "message"*/
        memset(&message, 0 , FRAME_SIZE);
        addr_size = sizeof(from_addr);
        if (recvfrom(sock, frame_buffer, FRAME_SIZE, 0, (struct sockaddr*)&from_addr, &addr_size) < 0)
            printf("Nothing received in %d\n", index);
        // else    
        //     printf("Received %s\n", message.data);
        memcpy(&message, frame_buffer , FRAME_SIZE);
        
        /* Lendo a mensagem para operar nela*/
        if(message.start == START){
            //printf("INCIANDO MENSAGEM\nBytes lidos: %d\nMessage: %s\n", index, message.data);

            /* Opera de acordo com a flag do jogo */
            /* Se atentar ao fato que não é sempre que se recebe o token, em shuffle você só escuta*/
            switch(message.flag){

                case SHUFFLE_FLAG: 
                    {                
                        if(message.dest == 0 && index == 0){
                            if(dest == 3){
                                MASTER_FLAG = BET_FLAG; 
                                dest = 0;   
                            }
                        }
                        else if (message.dest == index){
                            cartas_mao = message.num_cards;
                            deck = vetor_cartas(message.data, message.size, message.num_cards);
                            print_mao(deck, message.num_cards, round);
                        } 
                    }
                    break; 

                case BET_FLAG:
                    {
                        if(message.dest == 0 && index == 0){
                            printf("&&&&&& TODAS AS APOSTAS DA MESA &&&&&\n");  
                            print_apostas(message.data, message.size - 1, cartas_mao);
                            printf("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");  
                            converte_apostas(message.data, message.size, apostas, 4);
                            MASTER_FLAG = ALL_BETS_FLAG;
                        }
                        else{
                            print_apostas(message.data, message.size - 1, cartas_mao);
                            printf("\n");
                        }
                    }
                    break; 

                case ALL_BETS_FLAG:
                    {
                        
                        if(message.dest == 0 && index == 0){
                            MASTER_FLAG = MATCH_FLAG;
                        }
                        else{
                            printf("&&&&&& TODAS AS APOSTAS DA MESA &&&&&\n");  
                            print_apostas(message.data, message.size - 1, cartas_mao);
                            printf("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");  
                        }
                    }
                    break; 

                case MATCH_FLAG:
                    {
                        print_mesa(message.data, message.size - 1);
                        if(message.dest == 0 && index == 0){
                            converte_rodada(message.data, message.size, rodada, 4);
                            ganhador  = calcula_vitoria(rodada, vitorias, 4, gato);
                            MASTER_FLAG = RESULTS_FLAG;
                        }
                    }
                    break; 

                case RESULTS_FLAG:
                    {
                        print_resultado_rodada(message.data, message.size - 1, message.round);

                        if(message.dest == 0 && index == 0){

                            if(round == cartas_mao){
                                descontar_vidas_perdidas(vidas, apostas, vitorias, 4);                                
                                MASTER_FLAG = MATCH_RESULTS_FLAG; 
                            }
                            else{
                                round++;
                                MASTER_FLAG = MATCH_FLAG;
                            }
                        }
                    }
                    break;
                case MATCH_RESULTS_FLAG: 
                    {
                        //printf("@@@@@ %s @@@@\n", message.data);
                        if(contVidas != 0){
                            contVidas = vida_final_partida(message.data, message.size-1, contVidas, index, cartas_mao);
                            free(deck);
                        }

                        print_resultado_partida(message.data, message.size-1, cartas_mao);

                        if(message.dest == 0 && index == 0){
                                printf("Continuar a jogar?\n");
                                printf("[1] - Não\n[Qualquer Tecla] - Sim\n");
                                scanf("%d", &opt);
                                if(opt == 1){
                                    MASTER_FLAG = END_GAME_FLAG;
                                }
                                else{
                                    round = 1; 
                                
                                    if(cartas_mao ==  MAX_CARTAS_MAO)
                                         cartas_mao = 1; 
                                    else
                                         cartas_mao++;

                                    MASTER_FLAG = SHUFFLE_FLAG;
                                    memset(baralho, 0, TAM_BARALHO*sizeof(unsigned int));
                                    memset(vitorias, 0, NUM_NODES*sizeof(uint8_t));
                                }
                        }
                    }
                    break; 
                case END_GAME_FLAG:
                    {
                        
                        print_fim_jogo(message.data, message.size-1, message.num_cards);
                        if(message.dest == 0 && index == 0){
                            
                            free(vidas);
                            free(apostas);
                            free(vitorias);
                            free(rodada);
                            free(baralho);

                        }
                    }   
                    break; 
                default:
                    break;
            }

        }        

        receber_token(sock, &node_token , from_addr, addr_size);

        if(index == 0 && message.dest == 0 && message.flag == END_GAME_FLAG){
            break;
        }

    }
       
    close(sock);
}