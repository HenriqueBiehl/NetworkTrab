#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#include "lib_token_ring.h"

void cabecalhoAutores() {
    printf("***********************************\n");
    printf("*              Autores            *\n");
    printf("***********************************\n");
    printf("*                                 *\n");
    printf("*   Caio Henrique Ramos Rufino    *\n");
    printf("*          GRR20224386            *\n");
    printf("*                                 *\n");
    printf("*   Frank Wolff Hannemann         *\n");
    printf("*          GRR20224758            *\n");
    printf("*                                 *\n");
    printf("*   Henrique de Oliveira Biehl    *\n");
    printf("*          GRR20221257            *\n");
    printf("*                                 *\n");
    printf("***********************************\n");
}

int main(int argc, char *argv[]){
    
    if((argc != 2) && (argc != 4)){
        printf("ERRO: argumentos inválidos, utilize:\n");
        printf("    1. USO EM LOOPBACK:             ./dane-se <index 0 a 3>\n");
        printf("    2. USO COM MÚLTIPLAS MÁQUINAS:  ./dane-se <index 0 a 3> <ip do próximo jogador> <port>\n");
        return 1;
    }

    cabecalhoAutores();
    header_jogo_dane_se();

    int sock, index; 
    struct sockaddr_in my_addr, next_node_addr, from_addr;   
    uint8_t dest, MASTER_FLAG;
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

    snprintf(token, TOKEN_SIZE+1,"3SAwABfnXZAPSr9zIjoWtA4rcJNRcZjSSYlLnBcSKwpthrOc9Tv7xNrIYrxzcqi6");

    unsigned int *baralho;
    struct carta_t *deck;

    index = atoi(argv[1]);
    int next_node_index = (index + 1) % NUM_NODES;

    if(argc == 4){
        printf("Modo multiplayer\n");
        port = atoi(argv[3]);
        setar_nodo_mult_maquinas(&next_node_addr, argv[2], port);
    }
    else {
        port += index;
        setar_nodo_loop_back(&next_node_addr, next_node_index);
    }

    sock = socket(PF_INET, SOCK_DGRAM,0 );
    if(sock == -1){
        perror("Falha ao criar socket\n");
        exit(1);
    }

    if(!bind_socket(sock, &my_addr, port)){
        perror("Falha no bind do socket\n");
        close(sock);
        exit(1);
    }
    
    socklen_t addr_size;

    int opt; 

    printf("\n@@@@@@@@@@@@ INICIAR JOGO: JOGADOR %d @@@@@@@@@@@@\n", index);
    printf("\n[1]- Sim\n[2]- Sair\nEscolha:");
    scanf("%d", &opt);
    printf("\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
    if(opt == 2){
        close(sock);
        return 0;
    }

    srand(time(NULL));

    //Indíce 0 age como carteador e mestre da partida sempre
    if(index == 0){
        MASTER_FLAG = SHUFFLE_FLAG;      //Váriavel que define o comportamento que o mestre do jogo deve ter
        dest = 0;                       //Marcador que indica o destino que 0 deve enviar as cartas embaralhadas
        strcpy(node_token.token, token);
        memset(&message, 0 , FRAME_SIZE);

        //Vetor que 0 gerencia contendo a vida dos jogadores (cada indice é um jogador). Inicia com o valor de VIDA_MAX
        vidas = malloc(sizeof(uint8_t)*NUM_NODES);
        memset(vidas, VIDAS_MAX, NUM_NODES);

        //Vetor que gerencia as apostas dos jogadores (cada indice é um jogador)
        apostas = malloc(sizeof(uint8_t)*NUM_NODES);
        memset(apostas, 0, NUM_NODES);

        //Vetor que gerencia as vitorias dos jogadores (cada indice é um jogador)
        vitorias = malloc(sizeof(uint8_t)*NUM_NODES);
        memset(vitorias, 0, NUM_NODES);
        
        //Vetor que gerencia as rodada as cartas jogadas pelos jogadores (cada indice é um jogador)
        rodada = malloc(sizeof(struct carta_t)*NUM_NODES);
        memset(rodada, 0, sizeof(struct carta_t)*NUM_NODES);
        
        //Vetor que gerencia o baralho de TAM_BARALHO. Cada indice é uma carta e o baralho é inicializado como 0 
        //Significando que nenhuma carta esta sendo utlizada
        baralho = malloc(sizeof(unsigned int)*TAM_BARALHO);
        memset(baralho, 0, TAM_BARALHO*sizeof(unsigned int));
    }

    struct carta_t carta, gato;
    int cartas_mao = 1;              //Marca quantas cartas há na "mão" da rodada
    int round = 1;                   //Marca em qual rodada da "mão" o jogo esta
    int aposta;                      //Salva apostas dos jogadores
    int ganhador;                    //Salva o indice do ganhador da rodada
    uint8_t contVidas = VIDAS_MAX;   //Contador das vidas dos jogadores 

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
                                    print_mao(deck, cartas_mao, round);

                                    printf("Escolha uma carta para jogar\n");
                                    opt = escolhe_cartas(deck, cartas_mao);
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
                                    snprintf(data_buffer,33 ,"RECEBIDO:%c ROUND %c e %c CARTAS\n", converte_int_char(index),  converte_int_char(round), converte_int_char(cartas_mao));
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
                                    print_mao(deck, cartas_mao, round);
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
                            
                            if(contVidas == 0)
                                contVidas = VIDAS_MAX;
                            
                            cartas_mao = message.num_cards;
                            round = message.round;
                            deck = vetor_cartas(message.data, message.size, cartas_mao);
                            print_mao(deck, cartas_mao, round);
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
                        print_resultado_rodada(message.data, message.size - 1, round);

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

                                    if(unico_sobrevivente(vidas, 4)){
                                        memset(vidas, VIDAS_MAX, NUM_NODES);
                                        contVidas = VIDAS_MAX;
                                    }

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
    return 0;
}