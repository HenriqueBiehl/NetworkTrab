#include <stdio.h>
#include "comandos.h"

uint8_t get_comando() {

        printf("Envie uma mensagem ao servidor (listar/baixar):\n");

        char input[128];
        //gets(input);
        fgets(input, 128, stdin);

        if (strncmp(input, "listar", 6) == 0) {
                return CMD_LISTAR;
        } else if (strncmp(input, "baixar", 6) == 0) {
                return CMD_BAIXAR;
        } else if (strncmp(input, "exit", 4) == 0) {
                return CMD_SAIR;
        }

        return CMD_DESCONHECIDO;
}

int client_listar(int sckt, struct sockaddr_ll server_addr) {

        // Gera a mensagem para enviar ao servidor
        struct networkFrame message = gerar_mensagem_lista(0);
        printFrame(message);

        // Envia a mensagem lista pro servidor
        //int ret = sendto(sckt, (char*)&message, FRAME_SIZE, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        int ret = sendto(sckt, (char*)&message, FRAME_SIZE, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (ret < 0) {
                //fprintf(stderr, "Falha ao fazer sendto()");
                perror("Falha: ");
                //printf("ret = %d", ret);
                return -1;
        } else {
                printf("Mensagem enviada, aguardando servidor\n");
        }

        socklen_t add_len = sizeof(struct sockaddr_in);
        struct networkFrame received;

        // Agora deve receber a mensagem
        int rec = recvfrom(sckt, (char *)&received, FRAME_SIZE, 0, (struct sockaddr *)&server_addr, &add_len);
        if (rec < 0) {
                perror("Erro ao receber mensagem");
                close(sckt);
                return -1;
        } else {
                printf("Mensagem recebida com sucesso:\n");
                printFrame(received);
        }

        while (received.type != FIM_TX) {
                printf("Mostrando...\n");
                receber_mensagem_mostrar_tela(received);
                rec = recvfrom(sckt, (char*)&received, FRAME_SIZE, 0, (struct sockaddr *)&server_addr, &add_len);
                if (rec < 0) {
                        perror("Erro ao receber mensagem");
                        close(sckt);
                        /* Caso de NACK*/
                        return -1;
                }
        }

        printf("Fim da transmissao, listado o diretorio\n");
        printf("enviando ACK\n");

        int seq = 0;

        //printFrame(received);
        message = gerar_mensagem_ack(seq++);
        ret = sendto(sckt, (char*)&message, FRAME_SIZE, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (ret < 0) {
                fprintf(stderr, "Falha ao fazer sendto()\n");
                return -1;
        } else {
                printFrame(message);
                printf("Ack enviado\n");
        }

        return 0;
}

int client_baixar(int sckt, struct sockaddr_ll server_addr) {

        char nome_arquivo[64]; 
        printf("Digite o nome do arquivo:\n");
        //gets(nome_arquivo);
        fgets(nome_arquivo, MAX_DATA_LENGHT, stdin);
        printf("%s", nome_arquivo);
        //for(int i=0; i < strlen(nome_arquivo); ++i){
        //        printf("%d = %c\n", i, nome_arquivo[i]);
        //}
        if(nome_arquivo[strlen(nome_arquivo)-1] == '\n'){
                nome_arquivo[strlen(nome_arquivo)-1] = '\0';
        }

        //for(int i=0; i < strlen(nome_arquivo); ++i){
        //        printf("%d = %c\n", i, nome_arquivo[i]);
        //}

        //printf("%s", nome_arquivo);
        int seq = 0;

        struct networkFrame message = gerar_mensagem_baixar(seq, nome_arquivo, strlen(nome_arquivo));

        int ret = sendto(sckt, (char*)&message, FRAME_SIZE, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (ret < 0) {
                fprintf(stderr, "Falha ao fazer sendto()");
                return -1;
        } else {
                printf("Mensagem enviada, aguardando servidor\n");
        }

        struct networkFrame received;
        socklen_t add_len = sizeof(struct sockaddr_ll);

        int rec = recvfrom(sckt, (char*)&received, FRAME_SIZE, 0, (struct sockaddr *)&server_addr, &add_len);
        if (rec < 0) {
                perror("Erro ao receber mensagem");
                close(sckt);
                /* Caso de NACK*/
                return -1;
        }
        printFrame(received);

        FILE *baixado = fopen(nome_arquivo, "wb+");
        int count = 1;
        while (received.type != FIM_TX) {
                if (received.start != START) {
                        printf("mensagem invalida............\n");
                        continue;
                }
                printf("Baixando(%d)...\n", count++);
                printFrame(received);
                fwrite(received.data, sizeof(char), received.size, baixado);
                rec = recvfrom(sckt, (char*)&received, FRAME_SIZE, 0, (struct sockaddr *)&server_addr, &add_len);
                if (rec < 0) {
                        perror("Erro ao receber mensagem");
                        close(sckt);
                        /* Caso de NACK*/
                        return -1;
                }
        }

        //if (verifica_crc8((uint8_t*)&received, sizeof(received) - 1, received.crc8)) {
        //        printf("CRC deu certo\n");
        //} else {
        //        printf("CRC deu errado\n");
        //}
        printf("Recebeu o FIM_TX, fechando o arquivo ....n");
        fclose(baixado);

        //Enviando ack
        message = gerar_mensagem_ack(seq++);
        ret = sendto(sckt, (char*)&message, FRAME_SIZE, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (ret < 0) {
                fprintf(stderr, "Falha ao fazer sendto()");
                return -1;
        } else {
                printFrame(message);
                printf("Ack enviado");
        }
        printFrame(received);

        return 0;
}
