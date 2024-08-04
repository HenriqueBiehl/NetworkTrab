#include <stdio.h>
#include "comandos.h"

// Utilidade (pode ser criado um utils.s/h depois...)
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
        if (sendto_verify(sckt, (char*)&message, FRAME_SIZE, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
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
                //printFrame(received);
        }

        while (received.type != FIM_TX) {
                //printf("Mostrando...\n");
                receber_mensagem_mostrar_tela(received);
                rec = recvfrom(sckt, (char*)&received, FRAME_SIZE, 0, (struct sockaddr *)&server_addr, &add_len);
                if (rec < 0) {
                        perror("Erro ao receber mensagem");
                        close(sckt);
                        /* Caso de NACK*/
                        return -1;
                }
        }

        //printf("Fim da transmissao, listado o diretorio\n");
        //printf("enviando ACK\n");

        int seq = 0;

        //printFrame(received);
        message = gerar_mensagem_ack(seq++);

        if (sendto_verify(sckt, (char*)&message, FRAME_SIZE, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
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

        if (sendto_verify(sckt, (char*)&message, FRAME_SIZE, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
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
        } else {
                printf("recebida a primeira mensagem do baixar\n");
                printFrame(received);
        }

        FILE *baixado = fopen(nome_arquivo, "wb+");
        int count = 1;
        while (received.type != FIM_TX) {
                //if (received.type != DADOS) {
                //        printf("Esta mensagem nao é de dados.");
                //        continue;
                //}
                if (received.start != START) {
                        printf("mensagem invalida............\n");
                        continue;
                }
                if (received.type == DADOS) {
                        printf("Baixando(%d)...\n", count++);
                        printFrame(received);
                        fwrite(received.data, sizeof(char), received.size, baixado);
                }
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

        if (sendto_verify(sckt, (char*)&message, FRAME_SIZE, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
                printFrame(message);
                printf("Ack enviado");
        }
        //printFrame(received);

        return 0;
}

int server_listar(int sckt, struct sockaddr_ll client_addr) {

        //printf("LISTA\n");
        FILE *buffer_lista = listar_no_buffer();
        char line[MAX_DATA_LENGHT];
        int i = 0;
        while (fgets(line, sizeof(line), buffer_lista)) {
                //printf("&& %s\n", line);
                struct networkFrame mensagem_atual = gerar_mensagem_enviar_mostra_tela(line, i++);
                //printFrame(mensagem_atual);

                sendto_verify(sckt, (char *)&mensagem_atual, FRAME_SIZE, (struct sockaddr *)&client_addr, sizeof(client_addr));
                //printf("%s", line);
        } 
        struct networkFrame mensagem_fim_tx = gerar_mensagem_fim_tx(i++);
        //printf("enviando fim da tx\n");
        sendto_verify(sckt, (char *)&mensagem_fim_tx, FRAME_SIZE,(struct sockaddr *)&client_addr, sizeof(client_addr));

        fclose(buffer_lista);
        return 0;
}

int server_baixar(int sckt, struct sockaddr_ll client_addr, struct networkFrame message) {

        printf("BAIXAR: %s\n", message.data);
        char arq_path[128];
        char name_buffer[63];
        char buffer[63];
        FILE *arq;                        

        memset(name_buffer, 0, 63);
        strcpy(arq_path, DIR_CONTEUDOS);
        memcpy(name_buffer, message.data, message.size);
        strcat(arq_path, name_buffer);


        printf("%s", arq_path);
        for (int i = 0; i < strlen(arq_path); i++) {
                printf("%c ", arq_path[i]);
        }

        // Abre o arquivo em read binary
        arq = abrir_arquivo(arq_path, "rb");
        size_t bytes_read;
        memset(buffer, 0, 63);

        int i = 0;
        if(!arq) {
                printf("Erro: nao encontrado\n");
                snprintf(name_buffer, 15,"Nao econtrado\n");
                struct networkFrame mensagem_erro = gerar_mensagem_erro(i, name_buffer);
                printf("enviando mensagem_erro\n");
                //memcpy(msg, &mensagem_erro, FRAME_SIZE);
                //
                sendto_verify(sckt, (char*)&mensagem_erro, FRAME_SIZE, (struct sockaddr *)&client_addr, sizeof(client_addr));
        } else {                                        
                bytes_read = fread(buffer, sizeof(char), MAX_DATA_LENGHT, arq);
                struct networkFrame mensagem_dados = gerar_mensagem_dados(i, buffer, bytes_read);
                printf("enviando mensagem_dados %d\n", i);
                printFrame(mensagem_dados);

                if (sendto_verify(sckt, (char*)&mensagem_dados, FRAME_SIZE, (struct sockaddr *)&client_addr, sizeof(client_addr))){
                        i++;
                }

                while (!feof(arq)) {
                        bytes_read = fread(buffer, sizeof(char), MAX_DATA_LENGHT, arq);
                        struct networkFrame mensagem_dados = gerar_mensagem_dados(i, buffer, bytes_read);
                        printf("enviando mensagem_dados %d\n", i);
                        printFrame(mensagem_dados);
                        if (sendto_verify(sckt, (char*)&mensagem_dados, FRAME_SIZE, (struct sockaddr *)&client_addr, sizeof(client_addr))){
                                i++;
                        }
                }
                struct networkFrame mensagem_fim_tx = gerar_mensagem_fim_tx(i++);
                printf("ENVIANDO O FIM DA TX........................\n");

                //printFrame(mensagem_fim_tx);
                sendto_verify(sckt, (char*)&mensagem_fim_tx, FRAME_SIZE, (struct sockaddr *)&client_addr, sizeof(client_addr));

                fclose(arq);
        }

        return 0;
}
