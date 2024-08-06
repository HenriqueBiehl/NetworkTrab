#include "lib_token_ring.h"

int bind_socket(int sock, struct sockaddr_in *addr, unsigned int port){
    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = htonl(INADDR_ANY);
    addr->sin_port = htons(port);

    if(bind(sock, (struct sockaddr*)addr, sizeof(*addr)) == -1)
        return 0; 

    return 1;
}

void setar_nodo_mult_maquinas(struct sockaddr_in *node, char *ip_next_node, unsigned short port){
    memset(node, 0, sizeof(*node));
    node->sin_family = AF_INET;
    node->sin_port = htons(port);
    if (inet_pton(AF_INET, ip_next_node, &node->sin_addr) <= 0) {
        perror("invalid address/ Address not supported");
    }

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

void receber_token(int sock, struct token_ring *node_token ,struct sockaddr_in from_addr, socklen_t addr_size){
    char token_buffer[TOKEN_RING_SIZE+1];

    //printf("Recebendo o token\n");
    if (recvfrom(sock, token_buffer, TOKEN_RING_SIZE, 0, (struct sockaddr*)&from_addr, &addr_size) < 0)
        perror("No token received\n");            
    memcpy(node_token, token_buffer, TOKEN_RING_SIZE);
}
