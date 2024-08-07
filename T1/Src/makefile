# Compilador e flags
CC = gcc
CFLAGS = -std=gnu17 -Wall -g -Wno-packed-bitfield-compat -Wno-format-truncation -Wno-unused-variable -Wno-unused-value -D_XOPEN_SOURCE=500

# Diretórios
SRC_DIR = .
OBJ_DIR = obj

# Arquivos de código-fonte
SRCS = $(wildcard $(SRC_DIR)/*.c)

# Arquivos de cabeçalho (para dependências)
HEADERS = $(wildcard $(SRC_DIR)/*.h)

# Arquivos objeto
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Alvos (executáveis)
TARGETS = client server

# Alvo padrão
all: $(TARGETS)

# Alvo para compilar com LOOPBACK definido
lo: CFLAGS += -DLOOPBACK
lo: clean all

# Regra para construir o executável do client
client: $(OBJ_DIR)/client.o $(OBJ_DIR)/network.o $(OBJ_DIR)/comandos.o
	$(CC) $^ -o $@ $(CFLAGS)

# Regra para construir o executável do server
server: $(OBJ_DIR)/server.o $(OBJ_DIR)/network.o $(OBJ_DIR)/comandos.o
	$(CC) $^ -o $@ $(CFLAGS)

# Regra para compilar arquivos .c em arquivos .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	@mkdir -p $(OBJ_DIR)
	$(CC) -c $< -o $@ $(CFLAGS)

# Limpar arquivos gerados
clean:
	rm -rf $(OBJ_DIR) $(TARGETS)

.PHONY: clean lo

