#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h> 
#include <net/ethernet.h> 
#include <linux/if_packet.h> 
#include <net/if.h> 
#include <stdint.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>

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


FILE *listar_no_buffer() {
        FILE *fp;
        //char cmd[64] = sprintf("ls %s", DIR_CONTEUDOS);
        fp = popen("ls conteudos", "r");
        if (fp == NULL) {
                perror("popen");
                exit(EXIT_FAILURE);
        }
        return fp;
}

struct networkFrame gerar_mensagem_enviar_nome(char *nome, uint8_t seq) {

        struct networkFrame message;
        message.start = START;
        message.size = strlen(nome);
        message.seq = seq;
        message.type = LISTA;
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
        //char msg[MAX_DATA_LENGHT];
        char pack[FRAME_SIZE];

        bind_raw_socket(sckt, "lo");
        setar_modo_promiscuo(sckt, "lo");

        printf("sckt: %d\n", sckt);
        
        struct sockaddr_ll client_addr;
        socklen_t addr_len = sizeof(struct sockaddr_in);

        //int ret = recvfrom(sckt, pack, FRAME_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
        int ret;
        int i;
        while(1) {
                ret = recvfrom(sckt, pack, FRAME_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
                if (ret < 0) {
                        perror("Erro ao receber mensagem");
                        close(sckt);
                        return -1;
                } else {
                        printf("Received packet from %s:\n", client_addr.sll_addr);
                        //exit(1);
                }
                //printf("got it pal\n");
                //printf("%d bytes read. I shouldve got %d\n", ret, FRAME_SIZE);
                memcpy(&message, pack, FRAME_SIZE);
                //printf("Type = %u\n", message.type);
                //printFrame(message);
                switch(message.type) {
                        case(ACK):
                                printf("ACK, esperando proxima mensagem\n");
                                break;
                        case(MOSTRA_NA_TELA):
                                printf("Mostrar na tela\n");
                                break;
                        case(LISTA):
                                printf("LISTA\n");
                                char msg[FRAME_SIZE];
                                //int ifindex = if_nametoindex("lo");
                                FILE *buffer_lista = listar_no_buffer();

                                char line[256];
                                i = 0;
                                while (fgets(line, sizeof(line), buffer_lista)) {
                                        struct networkFrame mensagem_atual = gerar_mensagem_enviar_nome(line, i++);
                                        memcpy(msg, &mensagem_atual, FRAME_SIZE);
                                        ret = sendto(sckt, msg, FRAME_SIZE, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
                                        if(ret < 0){
                                                fprintf(stderr, "Falha ao fazer sendto()");
                                                return -1;
                                        }
                                        //printf("%s", line);
                                } 
                                struct networkFrame mensagem_fim_tx = gerar_mensagem_fim_tx(i++);
                                printf("enviando fim da tx\n");
                                memcpy(msg, &mensagem_fim_tx, FRAME_SIZE);
                                ret = sendto(sckt, msg, FRAME_SIZE, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
                                if(ret < 0){
                                        fprintf(stderr, "Falha ao fazer sendto()");
                                        return -1;
                                }

                                fclose(buffer_lista);
                                //exit (0); // Exit se nao buga e crasha...
                                //printFrame(mensagem);
                                break;
                        case(BAIXAR):
                                printf("BAIXAR: %s\n", message.data);
                                char arqNome[64];
                                char buffer[63];
                                FILE *arq;                        


                                memset(arqNome,0,64);
                                memset(buffer,0,63);
                                memcpy(arqNome, message.data, message.size);
                                arq = abrir_arquivo(arqNome, "r");
                                i = 0;
                                if(!arq){
                                        printf("Erro: nao encontrado\n");
                                        snprintf(buffer,15,"Nao econtrado\n");
                                        struct networkFrame mensagem_erro = gerar_mensagem_erro(i, buffer);
                                        printf("enviando mensagem_erro\n");
                                        memcpy(msg, &mensagem_erro, FRAME_SIZE);
                                        ret = sendto(sckt, msg, FRAME_SIZE, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
                                        if(ret < 0){
                                                fprintf(stderr, "Falha ao fazer sendto()");
                                                return -1;
                                        }
                                }
                                else{                                        
                                        fread(buffer, 63, MAX_DATA_LENGHT , arq);
                                        struct networkFrame mensagem_dados = gerar_mensagem_dados(i, buffer, ret);
                                        printf("enviando mensagem_dados %d\n", i);
                                        memcpy(msg, &mensagem_dados, FRAME_SIZE);
                                        ret = sendto(sckt, msg, FRAME_SIZE, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
                                        if(ret < 0){
                                                fprintf(stderr, "Falha ao fazer sendto()");
                                                return -1;
                                        }
                                        else 
                                                ++i;

                                        while (!feof(arq)) {
                                                ret = fread(buffer, 63, MAX_DATA_LENGHT, arq);
                                                struct networkFrame mensagem_dados = gerar_mensagem_dados(i, buffer, ret);
                                                printf("enviando mensagem_dados %d\n", i);
                                                memcpy(msg, &mensagem_dados, FRAME_SIZE);
                                                ret = sendto(sckt, msg, FRAME_SIZE, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
                                                if(ret < 0){
                                                        fprintf(stderr, "Falha ao fazer sendto()");
                                                        return -1;
                                                }
                                                else 
                                                        ++i;
                                        }

                                        struct networkFrame mensagem_fim_tx = gerar_mensagem_fim_tx(i++);
                                        printf("enviando fim da tx\n");
                                        memcpy(msg, &mensagem_fim_tx, FRAME_SIZE);
                                        ret = sendto(sckt, msg, FRAME_SIZE, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
                                        if(ret < 0){
                                                fprintf(stderr, "Falha ao fazer sendto()");
                                                return -1;
                                        }

                                        fclose(arq);
                                }
                                break;
                }

                /* CRC8 esta dando problema */
                /*if (verifica_crc8((uint8_t*)&message, sizeof(message) - 1, message.crc8)) {
                        printf("crc deu boinas\n");
                }
                else {
                        printf("num deu o crc\n");
                }*/
        }

        close(sckt);
        return 1;
}
