#ifndef COMANDOS_H
#define COMANDOS_H

#include "network.h"

#define CMD_LISTAR 1
#define CMD_BAIXAR 2
#define CMD_SAIR 3
#define CMD_DESCONHECIDO 4
#define TAM_JANELA 5

uint8_t get_comando();

int client_listar(int sckt, struct sockaddr_ll server_addr);

int client_baixar(int sckt, struct sockaddr_ll server_addr);

int client_baixar_janela_deslizante(int sckt, struct sockaddr_ll server_addr);

int server_listar(int sckt, struct sockaddr_ll client_addr);

int server_baixar(int sckt, struct sockaddr_ll client_addr, struct networkFrame message);

int server_baixar_janela_deslizante(int sckt, struct sockaddr_ll client_addr, struct networkFrame message);

#endif // COMANDOS_H
