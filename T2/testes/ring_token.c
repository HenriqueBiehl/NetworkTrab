#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT_BASE 5000
#define BUF_SIZE 256
#define NUM_NODES 4
#define MAX_DATA_LENGHT 2048
#define TOKEN_SIZE 64

struct message_frame{
    unsigned int start;  
    unsigned int size; 
    char gato[4];
    unsigned int apostas[4];
    unsigned int vidas[4];
    unsigned char data[2048]; 
};


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
    char token[TOKEN_SIZE+1];
    char buffer[TOKEN_SIZE+1];
    memset(buffer, 0, TOKEN_SIZE+1);
    snprintf(token, TOKEN_SIZE+1,"3SAwABfnXZAPSr9zIjoWtA4rcJNRcZjSSYlLnBcSKwpthrOc9Tv7xNrIYrxzcqi6");


    index = atoi(argv[1]);
    int next_node_index = (index + 1) % NUM_NODES;
    printf("Index: %d\n", index);

    sock = socket(PF_INET, SOCK_DGRAM,0 );
    if(sock == -1)
        perror("Falha ao criar socket\n");

    if(!bind_socket(sock, &my_addr, index))
        perror("Falha no bind do socket\n");
    
    printf("Socket para %d: %d\n", index, sock);
    setar_proximo_nodo(&next_node_addr, next_node_index);

    socklen_t addr_size;
    int opt; 
    int env; 

    while(1){
        printf("No while\n");
        env = recvfrom(sock, buffer, TOKEN_SIZE+1, 0, (struct sockaddr*)&from_addr, &addr_size);
        printf("%d\n", env);
        if(env < 0)
            perror("Falha ao fazer recvfrom()\n");
            
        if(strcmp(buffer, token) == 1){
            printf("Escolha a opção:\n[1] - Finalizar \n[2]- Transmitir\n[3] - Receber\n");
            scanf("%d", &opt);
        
            if(opt == 1)
                break; 
        
            if(opt == 2){
                printf("Transmitindo de %d para %d\n", index, next_node_index);
                snprintf(buffer, BUF_SIZE,"Bastão de %d\n", index);
                env =  sendto(sock, buffer, strlen(buffer) + 1, 0, (struct sockaddr*)&next_node_addr, sizeof(next_node_addr));
                if(env < 0) 
                    perror("Falha ao fazer sendto()\n");
            }   

            if(opt == 3){
                printf("Recebendo bastão em %d\n", index);
                addr_size = sizeof(from_addr);
                int str_len = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr*)&from_addr, &addr_size);
                if (str_len < 0)
                    printf("Nothing received in %d\n", index);
                else 
                    printf("MENSAGEM: %s\n", buffer);  
            }

             sendto(sock, buffer, TOKEN_SIZE+1, 0, (struct sockaddr*)&next_node_addr, sizeof(next_node_addr));
        }
         
    }

    close(sock);
}