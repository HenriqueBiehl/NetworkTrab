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

int main(){

        int sckt = cria_raw_socket();
        struct sockaddr_ll server_addr; 
        char msg[MAX_DATA_LENGHT] = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
        char transmission[FRAME_SIZE];

        bind_raw_socket(sckt, "lo");
        setar_modo_promiscuo(sckt, "lo");

        printf("sckt: %d\n", sckt);

        struct networkFrame message; 
        struct networkFrame message_copy; 

        message.start  = 0x7e;
        message.size   = 63;
        message.seq    = 1; 
        message.type   = MOSTRA_NA_TELA; //Só pra testar
        memcpy(&message.data, msg, MAX_DATA_LENGHT);
        message.crc8   = 255;

        //printf("%hhx ", message.start);
        //printf("%hhx ", message.size);
        //printf("%hhx ", message.seq); 
        //printf("%hhx ", message.type);
        //for(int i=0; i < MAX_DATA_LENGHT; ++i)
        //    printf("%hhx ", message.data[i]);
        //printf("%hhx ", message.crc8); 

        printf("Tamanho da struct networkFrame = %lu\n", sizeof(struct networkFrame));

        printBinary(message.start);
        printf("%u ", message.size);
        printf("\n");
        printf("%u ", message.seq); 
        printf("\n");
        printf("%u ", message.type);
        printf("\n");
        for(int i=0; i < MAX_DATA_LENGHT; ++i)
                printf("%c ", message.data[i]);
        printf("\n");
        printf("%u ", message.crc8); 


        memcpy(transmission, &message, FRAME_SIZE);
        memcpy(&message_copy, transmission, FRAME_SIZE);

        printf("COPIA:\n\n");

        printBinary(message_copy.start);
        printf("%u ", message_copy.size);
        printf("\n");
        printf("%u ", message_copy.seq); 
        printf("\n");
        printf("%u ", message_copy.type);
        printf("\n");
        for(int i=0; i < MAX_DATA_LENGHT; ++i)
                printf("%c ", message_copy.data[i]);
        printf("\n");
        printf("%u ", message_copy.crc8); 





        //copiar_frame_buffer(transmission, message);
        printf("Transmission in hex: \n");
        printf("%s\n", transmission);
        for(int i=0; i < FRAME_SIZE; ++i)
            printf("%hhx ", transmission[i]);
        printf("\n");

        //Procedimento para poder enviar informações
        int ifindex = if_nametoindex("lo");
        server_addr.sll_ifindex = ifindex;
        server_addr.sll_family = AF_PACKET;
        
        int ret = sendto(sckt, transmission, FRAME_SIZE, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if(ret < 0){
            fprintf(stderr, "Falha ao fazer sendto()");
            return -1;
        }
        else{
            printf("deu buenas kkk\nEnviei %d bytes\n", ret);
        }

        close(sckt);
        return 1;
}
