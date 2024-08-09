#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h> 
#include <net/ethernet.h> 
#include <linux/if_packet.h> 
#include <net/if.h> 
#include <stdint.h>

#include "comandos.h"
#include "network.h"

int main(){

        #ifdef LOOPBACK
                char *interface = "lo";
        #else
                char *interface = "eth0";
        #endif
        printf("interface = %s\n", interface);

        int sckt = cria_raw_socket();

        //memset(&server_addr, 0, sizeof(server_addr));
        bind_raw_socket(sckt, interface);
        setar_modo_promiscuo(sckt, interface);

        //printf("sckt: %d\n", sckt);

        //Procedimento para poder enviar informações
        //
        struct sockaddr_ll server_addr = {0};
        int ifindex = if_nametoindex(interface);
        server_addr.sll_ifindex = ifindex;
        server_addr.sll_family = AF_PACKET;
        server_addr.sll_protocol = htons(ETH_P_ALL);

        uint8_t comando;

        int running = 1;
        while (running) {
                comando = get_comando();
                switch (comando) {
                        case (CMD_LISTAR):
                                client_listar(sckt, server_addr);
                                break;

                        case (CMD_BAIXAR):
                                //client_baixar(sckt, server_addr);
                                client_baixar_janela_deslizante(sckt, server_addr);
                                break;

                        case (CMD_SAIR):
                                printf("Terminando a execução.\n");
                                running = 0;
                                break;

                        case (CMD_DESCONHECIDO):
                                printf("Comando inválido, tente novamente\n");
                                break;
                }
        }
        close(sckt);
        return 1;
}
