#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h> 
#include <net/ethernet.h> 
#include <linux/if_packet.h> 
#include <net/if.h> 
#include <stdint.h>

#include "network.h"

/*
   Cria um socket do tipo:
   - AF_PACKET para comunicação a nível de enlace
   - SOCK_RAW define o socket como rawsocket
   - htons(ETH_P_ALL) para capturar todos os protocolos Ethernet */
int cria_raw_socket(){
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

void copiar_frame_buffer(char *buffer, struct networkFrame frame){ 

        buffer[0] = frame.start; 
        buffer[1] = (frame.size & 0x3F) << 2 | (frame.seq >> 3);
        printf("%hhx e %hhx = ", frame.size, frame.seq);
        printf("%hhx\n", buffer[1]);
        buffer[2] = (frame.seq & 0x07) << 5 | (frame.type & 0x1F); 
        printf("%hhx e %hhx = ", frame.seq, frame.type);
        printf("%hhx\n", buffer[2]);
        for(int i = 3; i <= MAX_DATA_LENGHT + 3; ++i)
                buffer[i] = frame.data[i-3];
        buffer[67] = frame.crc8;
}

// recebe um do tipo lista e imprime na tela o nome do arquivo recebido
void receber_mensagem_lista(struct networkFrame frame) {
        printf("%s", frame.data);
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

struct networkFrame gerar_mensagem_ack(uint8_t seq) {

        struct networkFrame message; 
        message.start  = 0x7e;
        message.size   = 63;
        message.seq    = seq; 
        message.type   = ACK; //Só pra testar
        char msg[MAX_DATA_LENGHT] = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
        memcpy(&message.data, msg, MAX_DATA_LENGHT);
        message.crc8 = calcula_crc8((uint8_t*)&message, sizeof(message) - 1);

        return message;
}

int main(){

        int sckt = cria_raw_socket();
        struct sockaddr_ll server_addr; 
        //char msg[MAX_DATA_LENGHT] = "000000000000000000000000000000000000000000000000000000000000000";
        char transmission[FRAME_SIZE];

        bind_raw_socket(sckt, "lo");
        setar_modo_promiscuo(sckt, "lo");

        printf("sckt: %d\n", sckt);

        struct networkFrame message; 

        memcpy(transmission, &message, FRAME_SIZE);

        //Procedimento para poder enviar informações
        int ifindex = if_nametoindex("lo");
        server_addr.sll_ifindex = ifindex;
        server_addr.sll_family = AF_PACKET;

        int seq = 0;

        printf("Envie uma mensagem ao servidor (listar, baixar \"arquivo\"):\n");
        while (1) {
                printf("Esperando comando: ");
                char input[128];
                scanf("%s", input);
                //printf("%s", input);
                if (strncmp(input, "listar", 6) == 0) {
                        printf("Listando diretorio:\n");
                        message = gerar_mensagem_lista(seq++);
                        int ret = sendto(sckt, (char*)&message, FRAME_SIZE, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                        if (ret < 0) {
                                fprintf(stderr, "Falha ao fazer sendto()");
                                return -1;
                        } else {
                                printf("Mensagem enviada, aguardando servidor\n");
                        }
                        // Enviar a mensagem para listar
                } else if (strncmp(input, "baixar", 6) == 0) {
                        // Enviar a mensagem para baixar
                        exit(0);
                } else if (strcmp(input, "exit") == 0) {
                        exit(0);
                } else {
                        printf("Comando inexistente.\n");
                        exit(0);
                }

                char pack[FRAME_SIZE];
                socklen_t add_len = sizeof(struct sockaddr_in);

                int rec = recvfrom(sckt, pack, FRAME_SIZE, 0, (struct sockaddr *)&server_addr, &add_len);
                if (rec < 0) {
                        perror("Erro ao receber mensagem");
                        close(sckt);
                        return -1;
                }

                struct networkFrame recieved;
                memcpy(&recieved, pack, FRAME_SIZE);

                switch (recieved.type) {
                        case (LISTA):
                                while (recieved.type != FIM_TX) {
                                        receber_mensagem_lista(recieved);
                                        rec = recvfrom(sckt, pack, FRAME_SIZE, 0, (struct sockaddr *)&server_addr, &add_len);
                                        if (rec < 0) {
                                                perror("Erro ao receber mensagem");
                                                close(sckt);
                                                return -1;
                                        }
                                        memcpy(&recieved, pack, FRAME_SIZE);
                                }

                                printf("Fim da transmissao, listado o diretorio\n");
                                printf("enviando ACK\n");

                                //printFrame(recieved);
                                message = gerar_mensagem_ack(seq++);
                                int ret = sendto(sckt, (char*)&message, FRAME_SIZE, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                                if (ret < 0) {
                                        fprintf(stderr, "Falha ao fazer sendto()");
                                        return -1;
                                } else {
                                        printFrame(message);
                                        printf("Ack enviado");
                                }

                                break;
                }
        }
        close(sckt);
        return 1;
}