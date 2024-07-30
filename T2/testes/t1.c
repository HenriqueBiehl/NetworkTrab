#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT_BASE 5000
#define BUF_SIZE 256
#define NUM_NODES 3

struct message_frame{
    unsigned int start; 
    unsigned int size; 
    unsigned int dest; 
    unsigned char *data; 
};



void error_handling(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <node_index>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int node_index = atoi(argv[1]);
    int next_node_index = (node_index + 1) % NUM_NODES;

    int sock;
    struct sockaddr_in my_addr, next_node_addr, from_addr;
    socklen_t addr_size;
    char buffer[BUF_SIZE];

    sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    int opt_val = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val)) == -1)
        error_handling("setsockopt() error");

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons(PORT_BASE + node_index);

    if (bind(sock, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1)
        error_handling("bind() error");

    memset(&next_node_addr, 0, sizeof(next_node_addr));
    next_node_addr.sin_family = AF_INET;
    next_node_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // Para simplicidade, usando loopback
    next_node_addr.sin_port = htons(PORT_BASE + next_node_index);

    if (node_index == 0) {
        // Nó inicial começa com o bastão
        snprintf(buffer, BUF_SIZE, "Token from node %d", node_index);
        sendto(sock, buffer, strlen(buffer) + 1, 0, (struct sockaddr*)&next_node_addr, sizeof(next_node_addr));
        printf("Node %d sent token to node %d\n", node_index, next_node_index);
    }

    while (1) {
        addr_size = sizeof(from_addr);
        int str_len = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr*)&from_addr, &addr_size);
        if (str_len < 0)
            error_handling("recvfrom() error");

        printf("Node %d received token: %s\n", node_index, buffer);

        sleep(1);  // Simulação de algum processamento

        snprintf(buffer, BUF_SIZE, "Token from node %d", node_index);
        sendto(sock, buffer, strlen(buffer) + 1, 0, (struct sockaddr*)&next_node_addr, sizeof(next_node_addr));
        printf("Node %d sent token to node %d\n", node_index, next_node_index);
    }

    close(sock);
    return 0;
}