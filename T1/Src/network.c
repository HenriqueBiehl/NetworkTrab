#include "network.h"

/*
   Cria um socket do tipo:
   - AF_PACKET para comunicação a nível de enlace
   - SOCK_RAW define o socket como rawsocket
   - htons(ETH_P_ALL) para capturar todos os protocolos Ethernet
   */
int cria_raw_socket() {
        int sckt = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
        if(sckt == -1){
                fprintf(stderr, "Erro ao criar raw socket, verifique se você é root");
                exit(-1);
        }

        return sckt;
}

void bind_raw_socket(int sckt, char *netInterface){
        /*
           Obtém o indice da interface de rede especificada pelo parametro
           netInterface
           */
        int ifindex = if_nametoindex(netInterface);

        struct sockaddr_ll endereco = {0};          
        endereco.sll_family = AF_PACKET;            //Identifica camada de enlace e o protocolo
        endereco.sll_protocol = htons(ETH_P_ALL);   //Marca que pode receber tudo do Ethernet
        endereco.sll_ifindex = ifindex;             //Recebe o indice de interface obtido anteriormente

        if(bind(sckt, (struct sockaddr*) &endereco, sizeof(endereco)) == -1){
                fprintf(stderr, "Erro no bind do socket");
                exit(-1);
        }
}


void setar_modo_promiscuo(int sckt, char *netInterface){
        int ifindex = if_nametoindex(netInterface);

        //Marca como Promiscuo, ou seja, ele pode aceitar todos os pacotes
        //e ler todos os pacotes independentemente de para quem ele é direcionado
        struct packet_mreq mr = {0}; 
        mr.mr_ifindex = ifindex;
        mr.mr_type = PACKET_MR_PROMISC;

        if(setsockopt(sckt, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1){
                fprintf(stderr, "Erro ao fazer setsockopt: \n verifique se a interface de rede foi especificada no diretório ");
                exit(-1);
        }
}

// Realiza o sendto e verifica o erro
int sendto_verify(int sckt, const void *message, size_t length, struct sockaddr *dest_addr, socklen_t dest_len) {

        int ret = sendto(sckt, message, length, 0, dest_addr, dest_len);
        if(ret < 0){
                perror("Falha ao fazer sendto()");
                return -1;
        }
        return ret;
}

struct networkFrame gerar_mensagem_lista(uint8_t seq) {

        struct networkFrame message; 
        message.start  = 0x7e;
        message.size   = 63;
        message.seq    = seq; 
        message.type   = LISTA; //Só pra testar
        char msg[MAX_DATA_LENGHT] = "LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL";
        memcpy(&message.data, msg, MAX_DATA_LENGHT);
        message.crc8 = calcula_crc8((uint8_t*)&message, sizeof(message) - 1);

        return message;
}

struct networkFrame gerar_mensagem_baixar(uint8_t seq, char *arqNome, int tam) {

        struct networkFrame message; 
        message.start  = 0x7e;
        message.size   = tam;
        message.seq    = seq; 
        message.type   = BAIXAR; //Só pra testar
        memset(message.data, 0, MAX_DATA_LENGHT);
        memcpy(&message.data, arqNome, tam);
        message.crc8 = calcula_crc8((uint8_t*)&message, sizeof(message) - 1);

        return message;

}

struct networkFrame gerar_mensagem_resposta(uint8_t seq, uint8_t type) {

        struct networkFrame message; 
        message.start  = 0x7e;
        message.size   = 63;
        message.seq    = seq; 
        message.type   = type; //Só pra testar
        char msg[MAX_DATA_LENGHT] = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
        memcpy(&message.data, msg, MAX_DATA_LENGHT);
        message.crc8 = calcula_crc8((uint8_t*)&message, sizeof(message) - 1);

        return message;
}

struct networkFrame gerar_mensagem_enviar_mostra_tela(char *nome, uint8_t seq) {

        struct networkFrame message;
        message.start = START;
        message.size = strlen(nome);
        message.seq = seq;
        message.type = MOSTRA_NA_TELA;
        memset(message.data, 0, MAX_DATA_LENGHT);
        memcpy(&message.data, nome, message.size);
        message.crc8 = calcula_crc8((uint8_t*)&message, sizeof(message) - 1);

        return message;
}

struct networkFrame gerar_mensagem_erro(uint8_t seq, char *erro){

        struct networkFrame message;
        message.start = START;
        message.size = strlen(erro);
        message.seq = seq;
        message.type = ERRO;
        memset(message.data, 0, MAX_DATA_LENGHT);
        memcpy(message.data, erro, strlen(erro));
        message.crc8 = calcula_crc8((uint8_t*)&message, sizeof(message) - 1);

        return message;
}

struct networkFrame gerar_mensagem_dados(uint8_t seq, char *data, uint8_t size){

        struct networkFrame message;
        message.start = START;
        message.size = size;
        message.seq = seq;
        message.type = DADOS;
        memset(message.data, 0, MAX_DATA_LENGHT);
        memcpy(message.data, data, size);
        message.crc8 = calcula_crc8((uint8_t*)&message, sizeof(message) - 1);

        return message;
}

struct networkFrame gerar_mensagem_fim_tx(uint8_t seq) {

        struct networkFrame message;
        message.start = START;
        message.size = MAX_DATA_LENGHT;
        message.seq = seq;
        message.type = FIM_TX;
        memset(message.data, 0, MAX_DATA_LENGHT);
        message.crc8 = calcula_crc8((uint8_t*)&message, sizeof(message) - 1);       
        return message;
}

// recebe um do tipo lista e imprime na tela o nome do arquivo recebido
void receber_mensagem_mostrar_tela(struct networkFrame frame) {
        printf("%s", frame.data);
}

void printBinary(uint8_t n) {
        // Define the number of bits in an unsigned int
        unsigned int numBits = sizeof(uint8_t) * 8;

        // Iterate through each bit (from the most significant to the least significant)
        for (int i = numBits - 1; i >= 0; i--) {
                // Extract the i-th bit and print it
                unsigned int bit = (n >> i) & 1;
                printf("%u", bit);
        }

        printf("\n"); // Move to the next line after printing the binary representation
}

void printFrame(struct networkFrame frame) {

        printBinary(frame.start);
        printf("Size: %u ", frame.size);
        printf("\n");
        printf("Seq: %u ", frame.seq); 
        printf("\n");
        printf("Type: %u ", frame.type);
        printf("\nData: ");
        for(int i=0; i < MAX_DATA_LENGHT; ++i)
                printf("%c ", frame.data[i]);
        printf("\n");
        printf("CRC: %u ", frame.crc8);
}

uint8_t calcula_crc8(uint8_t *data, size_t len) {
        uint8_t crc = 0x00;
        uint8_t polynomial = 0x07; // Polinômio padrão do CRC-8: x^8 + x^2 + x + 1

        for (size_t i = 0; i < len; i++) {
                crc ^= data[i];
                for (uint8_t j = 0; j < 8; j++) {
                        if (crc & 0x80) {
                                crc = (crc << 1) ^ polynomial;
                        } else {
                                crc <<= 1;
                        }
                }
        }
        return crc;
}

int verifica_crc8(uint8_t *data, size_t len, uint8_t crc_recebido) {
        uint8_t crc_calculado = calcula_crc8(data, len);
        return (crc_calculado == crc_recebido);
}

FILE *abrir_arquivo(char *nome, char *tipo)
{
        FILE *arq; 

        printf("tentando abrir %s\n", nome);
        arq = fopen(nome, tipo); 

        if(!arq) {
                printf("%s ", nome);
                perror("Erro ao abrir o arquivo  \n");
                return NULL;
        }

        return arq;
}


void lista_conteudos(){
        system("ls *.mp4 *.avi > lista");
}

void descritor_arquivo(char *nomeArquivo){
        char buf[1024];
        sprintf(buf, "ls -l %s > t1", nomeArquivo);
        system(buf);
        system("cut -d' ' -f5,6,7,8 t1 > descritor");
        system("rm t1");
}
