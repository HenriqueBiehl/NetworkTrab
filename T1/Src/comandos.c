#include "comandos.h"

// Função para empacotar informações do arquivo
void empacotar_info_stat(const char *nome_arquivo, char *buffer) {
        struct stat file_stat;

        if (stat(nome_arquivo, &file_stat) < 0) {
                perror("stat");
                return;
        }

        // Tipo de Arquivo (1 byte)
        buffer[0] = (file_stat.st_mode & S_IFDIR) ? 'D' : 'F';

        // Permissões (3 bytes)
        buffer[1] = (file_stat.st_mode & S_IRUSR) ? 'r' : '-';
        buffer[2] = (file_stat.st_mode & S_IWUSR) ? 'w' : '-';
        buffer[3] = (file_stat.st_mode & S_IXUSR) ? 'x' : '-';

        // Tamanho do Arquivo (7 bytes)
        snprintf(buffer + 4, 8, "%7ld", file_stat.st_size);

        // Data de Modificação (8 bytes)
        struct tm *timeinfo = localtime(&file_stat.st_mtime);
        snprintf(buffer + 11, 9, "%04d%02d%02d", timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday);

        // Horário de Modificação (6 bytes) - Hora e Minuto
        snprintf(buffer + 19, 6, "%02d%02d", timeinfo->tm_hour, timeinfo->tm_min);
}

// Função para desempacotar e aplicar as informações do arquivo
void desempacotar_info_stat(const char *buffer, const char *nome_arquivo) {
        // Cria o arquivo
        int fd = open(nome_arquivo, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
                perror("open");
                return;
        }

        // Tipo de Arquivo e Permissões
        mode_t mode = 0;
        if (buffer[0] == 'D') {
                mode |= S_IFDIR;
        } else if (buffer[0] == 'F') {
                mode |= S_IFREG;
        }

        if (buffer[1] == 'r') mode |= S_IRUSR;
        if (buffer[2] == 'w') mode |= S_IWUSR;
        if (buffer[3] == 'x') mode |= S_IXUSR;

        // Setando permissões para grupo e outros (Assumindo permissões iguais ao usuário)
        if (buffer[1] == 'r') mode |= S_IRGRP | S_IROTH;
        if (buffer[2] == 'w') mode |= S_IWGRP | S_IWOTH;
        if (buffer[3] == 'x') mode |= S_IXGRP | S_IXOTH;

        // Aplica as permissões do arquivo
        fchmod(fd, mode);

        // Tamanho do Arquivo
        char tamanhoStr[8];
        strncpy(tamanhoStr, buffer + 4, 7);
        tamanhoStr[7] = '\0'; // Finaliza com null
        off_t tamanhoArquivo = strtol(tamanhoStr, NULL, 10);

        // Trunca o arquivo para o tamanho desejado
        ftruncate(fd, tamanhoArquivo);

        // Data e Hora de Modificação
        char dataStr[9], horaStr[5];
        strncpy(dataStr, buffer + 11, 8);
        dataStr[8] = '\0'; // Finaliza com null

        strncpy(horaStr, buffer + 19, 4);
        horaStr[4] = '\0'; // Finaliza com null

        struct tm timeinfo = {0};
        strptime(dataStr, "%Y%m%d", &timeinfo);
        timeinfo.tm_hour = strtol(horaStr, NULL, 10) / 100;
        timeinfo.tm_min = strtol(horaStr, NULL, 10) % 100;

        time_t mod_time = mktime(&timeinfo);

        // Aplica a data de modificação
        struct utimbuf new_times;
        new_times.actime = mod_time;    // Tempo de acesso
        new_times.modtime = mod_time;   // Tempo de modificação
        utime(nome_arquivo, &new_times);

        // Fecha o arquivo
        close(fd);
}


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
                printf("recebida a primeira mensagem do baixar");

                // Essa mensagem deve ser do tipo descritor de arquivo
        }

        FILE *baixado = fopen(nome_arquivo, "wb+");
        int count = 1;
        while (received.type != FIM_TX) {
                if (received.start != START) {
                        printf("mensagem invalida............\n");
                        continue;
                }
                if (received.type == DESCRITOR_ARQUIVO) {
                        printf("Recebido o descritor do arquivo\n");
                        desempacotar_info_stat((const char *)received.data, nome_arquivo);
                        printFrame(received);
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

                empacotar_info_stat(arq_path, buffer);
                struct networkFrame mensagem_descritor = gerar_mensagem_descritor_arq(i, buffer);
                sendto_verify(sckt, (char*)&mensagem_descritor, FRAME_SIZE, (struct sockaddr *)&client_addr, sizeof(client_addr));
                printFrame(mensagem_descritor);

                memset(buffer, 0, 63);

                //criar loop para preencher a janela. Para quando a janela estiver preenchida ou o arquivo acabou       
                bytes_read = fread(buffer, sizeof(char), MAX_DATA_LENGHT, arq);
                struct networkFrame mensagem_dados = gerar_mensagem_dados(i, buffer, bytes_read);
                //Adicionar i a janela_deslizante
                printf("enviando mensagem_dados %d\n", i);
                printFrame(mensagem_dados);

                //Envia de acordo com tamanho da janela (e deve vir depois do loop)
                if (sendto_verify(sckt, (char*)&mensagem_dados, FRAME_SIZE, (struct sockaddr *)&client_addr, sizeof(client_addr))){
                        i++;
                }
                //Ao enviar, aguarda o ACK/NACK do cliente

                //Caso receba um NACK, reinicia i e deixa no vetor da janela as mensagens que ainda não foram confirmadas
                //Caso ACK, reinicia i e faz uma outra leitura em loop dessa (FIM_TX vai nessa tbm)
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

int server_baixar_janela_deslizante(int sckt, struct sockaddr_ll client_addr, struct networkFrame message) {

        printf("BAIXAR JANELA DESLIZANTES: %s\n", message.data);
        char arq_path[128];
        char name_buffer[63];
        char buffer[63];
        struct networkFrame window[5];
        FILE *arq;                        

        memset(window, 0, FRAME_SIZE*5);
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
                int checkpoint_i;         //Salva o indice do último elemento inserido na janela, útil para saber seu tamanho real
                int msg_over = 0;         //Flag que indica que o arquivo chegou ao fim
                int end_operation = 0;    //Flag que indica fim da operaçao de envio de arquivos
                int index_startpoint = 0; //Indica o ponto de inicio para o indice da janela deslizante
                int seq = 0;              //Indica o número da sequencia de msgs na janela

                while(!end_operation) {

                        for(int i = index_startpoint; i < TAM_JANELA && !feof(arq); i++){
                                bytes_read = fread(buffer, sizeof(char), MAX_DATA_LENGHT, arq);
                                window[i] = gerar_mensagem_dados(seq, buffer, bytes_read);
                                printf("Mensagem %d gerada\n", i);
                                printFrame(window[i]);
                                seq++;
                                checkpoint_i = i;
                        }

                        /* Significa que você não completou a janela mas fechou o arquivo, então, da pra enviar FIM_TX na janela*/
                        if (checkpoint_i < 4) {
                                checkpoint_i++;
                                window[checkpoint_i] = gerar_mensagem_fim_tx(seq);
                                printf("======ENVIANDO O FIM DA TX=======\n");
                                msg_over = 1;
                        }

                        //Envia as mensagens da janela 
                        for (int i = 0; i < checkpoint_i + 1; ++i) {
                                sendto_verify(sckt, (char*)&window[i], FRAME_SIZE, (struct sockaddr *)&client_addr, sizeof(client_addr));
                        }

                        struct sockaddr_ll client_addr = {0};
                        struct networkFrame client_answer;

                        socklen_t addr_len = sizeof(struct sockaddr_ll);
                        int ifindex = if_nametoindex("lo");
                        client_addr.sll_ifindex = ifindex;
                        client_addr.sll_family = AF_PACKET;
                        client_addr.sll_protocol = htons(ETH_P_ALL);

                        int ret = recvfrom(sckt, (char *)&client_answer, FRAME_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
                        if (ret < 0) {
                                perror("Erro ao receber mensagem");
                                close(sckt);
                                return -1;
                        } else {
                                //printFrame(client_answer);
                                //printf("Received packet from %s:\n", client_addr.sll_addr);
                        }

                        while(client_answer.start != START){
                                int ret = recvfrom(sckt, (char *)&client_answer, FRAME_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
                                if (ret < 0) {
                                        perror("Erro ao receber mensagem");
                                        close(sckt);
                                        return -1;
                                } else {
                                        //printFrame(client_answer);
                                        //printf("Received packet from %s:\n", client_addr.sll_addr);
                                }
                        }

                        switch(client_answer.type){
                                case ACK: 
                                        //Caso tenha acabado de ler o arquivo, marca o fim da operação
                                        if (msg_over) {
                                                end_operation = 1;
                                        } else {
                                                seq = (client_answer.seq + 1)%TAM_JANELA; //Avança em + 1 na janela em relação a ultima sequencia
                                        }
                                        break;

                                case NACK:
                                        seq = (seq + 1) % TAM_JANELA; //Sequencia avança em +1 na janela 
                                        index_startpoint = TAM_JANELA - client_answer.seq;
                                        //Puxar mensagens nao confirmadas para o inicio do vetor
                                        for(int i = client_answer.seq; i < TAM_JANELA && client_answer.seq != 0; ++i){
                                                window[i-client_answer.seq] = window[i];
                                        }
                                        break;
                        }
                        break;
                }
        }
        fclose(arq);        
        return 0;
}

int client_baixar_janela_deslizante(int sckt, struct sockaddr_ll server_addr) {

        char nome_arquivo[64];
        struct networkFrame window[5];

        memset(window, 0, FRAME_SIZE*5);

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
                printf("MENSAGEM BAIXAR ENVIADA, AGUARDANDO SERVIDOR\n");
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
        int seq_failure = 0;
        int seq_checkpoint = TAM_JANELA;
        int has_failures = 0; 
        int index_failure = TAM_JANELA;
        int fim_op = 0;

        while (1) {

                //Processar que nem sempre quem ele vai receber é de fato uma mensagem nova
                int received_window = 0; 
                while(received_window < TAM_JANELA) {

                        rec = recvfrom(sckt, (char*)&received, FRAME_SIZE, 0, (struct sockaddr *)&server_addr, &add_len);
                        if (rec < 0) {
                                perror("Erro ao receber mensagem");
                        } else {
                                printf("Recebida a janela %d\n", received_window);
                        }

                        if(received.start == START){
                                printf("A janela %d tem o start correto\n", received_window);
                                //int crc8 = verifica_crc8((uint8_t*)&received, sizeof(received) - 1, message.crc8);
                                //printf("crc8 calculado = %d, crc8 recebido = %d\n", crc8, message.crc8);
                                //if (crc8) {
                                if (verifica_crc8((uint8_t*)&received + 1, sizeof(received) - 2, received.crc8)) {
                                        seq_checkpoint = received.seq;
                                } else {
                                        has_failures = 1;
                                        seq_failure = (seq_checkpoint + 1) % TAM_JANELA; //Se deu falha, você nao garante que o dado de seq esta inteiro.
                                        index_failure = received_window;
                                        printf("A janela %d tem erro\n", received_window);
                                        break;
                                }

                                /* Significa que uma mensagem da sequencia deu problem na hora do envio (sequencia esta fora de ordem) */
                                if (window[received_window].seq != (seq_checkpoint + 1) % TAM_JANELA){
                                        has_failures = 1;
                                        seq_failure = (seq_checkpoint + 1) % TAM_JANELA; //Se deu falha, você nao garante que o dado de seq esta inteiro.
                                        index_failure = received_window;
                                        printf("Problema na hora do envio (seq fora de ordem\n)");
                                        break;
                                }

                                memcpy(&window[received_window], &received, FRAME_SIZE);
                                if(received.type == FIM_TX)
                                        break;

                                received_window++; //Diz quandos elementos vc recebeu na janela, serve como indice para a janela
                        }               
                }

                for(int i = 0; i < received_window && i < index_failure; ++i) {
                        if (window[i].type == DADOS) {
                                printf("Baixando(%d)...\n", count++);
                                printFrame(received);
                                fwrite(received.data, sizeof(char), received.size, baixado);
                        }
                        if (window[i].type == FIM_TX) {
                                fim_op = 1;
                                printf("A janela %d contem o FIM TX\n", i);
                        }
                }

                struct networkFrame answer; 
                if (has_failures) {
                        answer = gerar_mensagem_resposta(seq_failure, NACK);
                } else {
                        printf("Não houve erro, gerando mensagem de resposta");
                        answer = gerar_mensagem_resposta(seq_checkpoint, ACK);
                }

                if (sendto_verify(sckt, (char*)&answer, FRAME_SIZE, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
                        //printFrame(answer);
                        //printf("Resposta %d enviada", answer.type);
                }

                //printf("%d\n", fim_op);
                if (fim_op) {
                        break;
                }

                //Ler todas as mensagens na janela
                //Verificar se tem mensagens certas/Erradas
                //Enviar ACK/NACK 
                //Escrever o que da pra escrever
                //Repetir até FIM_TX

        } 

        //printf("Recebeu o FIM_TX, fechando o arquivo ....n");
        fclose(baixado);

        //Enviando ack
        /*message = gerar_mensagem_ack(seq++);

          if (sendto_verify(sckt, (char*)&message, FRAME_SIZE, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
          printFrame(message);
          printf("Ack enviado");
          }*/
        //printFrame(received);

        return 0;
}
