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
#include "comandos.h"

int main(){

        //const *interface;

        #ifdef LOOPBACK
                char *interface = "lo";
        #else
                char *interface = "eth0";
        #endif
        printf("interface = %s\n", interface);

        int sckt = cria_raw_socket();
        struct networkFrame message;

        bind_raw_socket(sckt, interface);
        setar_modo_promiscuo(sckt, interface);

        //printf("sckt: %d\n", sckt);

        struct sockaddr_ll client_addr = {0};

        socklen_t addr_len = sizeof(struct sockaddr_ll);

        int ifindex = if_nametoindex(interface);
        client_addr.sll_ifindex = ifindex;
        client_addr.sll_family = AF_PACKET;
        client_addr.sll_protocol = htons(ETH_P_ALL);

        int ret;
        //ret = recvfrom(sckt, message, FRAME_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
        while(1) {
                ret = recvfrom(sckt, (char *)&message, FRAME_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
                if (ret < 0) {
                        perror("Erro ao receber mensagem");
                        close(sckt);
                        return -1;
                } else {
                        printf("Received packet from %s:\n", client_addr.sll_addr);
                }
                if(message.start == START){
                        switch(message.type) {
                                case(ACK):
                                        printf("ACK, esperando proxima mensagem\n");
                                        break;
                                case(LISTA):
                                        server_listar(sckt, client_addr);
                                        break;

                                case(BAIXAR):
                                        //server_baixar(sckt, client_addr, message);
                                        server_baixar_janela_deslizante(sckt, client_addr, message);
                                        //exit(0);
                                        break;
                        }
                }

        }

        close(sckt);
        return 1;
}
