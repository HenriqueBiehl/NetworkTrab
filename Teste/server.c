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
   - htons(ETH_P_ALL) para capturar todos os protocolos Ethernet
   */
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
        buffer[1] = (frame.size) << 2 | frame.seq >> 3;
        printf("%hhx e %hhx = ", frame.size, frame.seq);
        printf("%hhx\n", buffer[1]);
        buffer[2] = frame.seq << 2 | frame.type; 
        printf("%hhx e %hhx = ", frame.seq, frame.type);
        printf("%hhx\n", buffer[2]);
        memcpy(buffer+16, frame.data, MAX_DATA_LENGHT);
        buffer[67] = frame.crc8;
}

int main(){

        int sckt = cria_raw_socket();
        struct networkFrame message;
        char msg[MAX_DATA_LENGHT];
        char pack[FRAME_SIZE];

        bind_raw_socket(sckt, "lo");
        setar_modo_promiscuo(sckt, "lo");

        printf("sckt: %d\n", sckt);

        while(1){
                int ret = recvfrom(sckt, pack, FRAME_SIZE, 0, NULL, NULL);
                if (ret < 0) {
                        perror("Erro ao receber mensagem");
                        close(sckt);
                        return -1;
                }
                printf("got it pal\n");
                printf("%d bytes read. I shouldve got %d\n", ret, FRAME_SIZE);

                memcpy(&message, pack, FRAME_SIZE);
                printFrame(message);
                
                if (verifica_crc8((uint8_t*)&message, sizeof(message) - 1, message.crc8)) {
                        printf("crc deu boinas\n");
                }
                else {
                        printf("num deu o crc\n");
                }

                //for(int i=0; i < FRAME_SIZE; ++i)
                //        printf("%hhx ", pack[i]);
                // printf("    size: %d\n", transmission.size);
                // printf("    sequency: %d\n", transmission.seq);
                // printf("    type: %d\n", transmission.type);
                // printf("    data: %s\n", transmission.data);
                // printf("    crc8: %d\n", transmission.crc8);
        }

        close(sckt);
        return 1;
}
