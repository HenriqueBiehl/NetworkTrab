#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h> 
#include <net/ethernet.h> 
#include <linux/if_packet.h> 
#include <net/if.h> 
#include <stdint.h>

#define MAX_DATA_LENGHT 63

struct networkFrame {
    uint8_t start; 
    uint8_t size:6;
    uint8_t seq:5;
    uint8_t type:5;
    unsigned char data[MAX_DATA_LENGHT];  
    uint8_t crc8;
};

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

int main(){

    int sckt = cria_raw_socket();
    struct sockaddr_ll server_addr; 
    char msg[MAX_DATA_LENGHT] = "AvO9AQdHSWyos9XfCBghO4Vy9mzR6RzAvO9AQdHSWyos9XfCBrghO4Vy9mzR6R";

    bind_raw_socket(sckt, "lo");

    printf("sckt: %d\n", sckt);

    struct networkFrame message; 

    message.start  = 0x7e;
    message.size   = 63;
    message.type   = 0x0; //Só pra testar
    memcpy(&message.data, msg, MAX_DATA_LENGHT);
    message.seq    = 1; 
    message.crc8   = 255;

    //Procedimento para poder enviar informações
    int ifindex = if_nametoindex("lo");
    server_addr.sll_ifindex = ifindex;
    server_addr.sll_family = AF_PACKET;
    
    int ret = sendto(sckt, (struct networkFrame*) &message,sizeof(struct networkFrame), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(ret < 0){
        fprintf(stderr, "Falha ao fazer sendto()");
        return -1;
    }
    else{
        printf("deu buenas kkk\n");
    }

    close(sckt);
    return 1;
}