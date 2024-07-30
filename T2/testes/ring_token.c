#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT_BASE 5000
#define BUF_SIZE 256
#define NUM_NODES 3

int bind_socket(int sock, struct sockaddr_in *addr, unsigned int index){
    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = htonl(INADDR_ANY);
    addr->sin_port = htons(PORT_BASE + index);

    if(bind(sock, (struct sockaddr*)addr, sizeof(*addr)) == -1)
        return 0; 

    return 1;
}

void setar_proximo_nodo(struct sockaddr_in *node, unsigned int index){
    memset(node, 0, sizeof(*node));
    node->sin_family = AF_INET;
    node->sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // Para simplicidade, usando loopback
    node->sin_port = htons(PORT_BASE + index);
}

int main(int argc, char *argv[]){
    int sock, index; 
    struct sockaddr_in my_addr, next_node_addr, from_addr;   
    char buffer[BUF_SIZE];

    index = atoi(argv[1]);
    int next_node_index = (index + 1) % NUM_NODES;

    sock = socket(PF_INET, SOCK_DGRAM, 0);
    if(sock == -1)
        perror("Falha ao criar socket\n");

    if(!bind_socket(sock, &my_addr, index))
        perror("Falha ao criar socket\n");
    
    printf("Socket para %d: %d\n", index, sock);
    setar_proximo_nodo(&next_node_addr, next_node_index);

    if(index == 0){
        snprintf(buffer, BUF_SIZE,"Bastão de %d\n", index);
        int env = sendto(sock, buffer, strlen(buffer) + 1, 0, (struct sockaddr*)&next_node_addr, sizeof(next_node_addr));
        if(env < 0) 
            perror("Falha ao fazer sendto()\n");
        
    }

    socklen_t addr_size;
    int opt; 
    while(1){
        printf("Waiting\n");
        addr_size = sizeof(from_addr);
        printf("Sending buffer\n");
        int str_len = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr*)&from_addr, &addr_size);
        if (str_len < 0)
            printf("Nothing received in %d\n", index);
        else 
            printf("Received: %s\n", buffer);  

        printf("Move on?\n");
        scanf("%d", &opt);
        if(opt)
            break;

        sleep(1);  // Simulação de algum processamento
    }

    close(sock);
}