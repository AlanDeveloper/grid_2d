# Makefile para Simulação Híbrida MPI + OpenMP

# diretório onde o executável será salvo
BIN_DIR = bin

# caminho completo do executável final
TARGET = $(BIN_DIR)/simulation

# nome do arquivo fonte
SRC = simulation_base.c

# compilador e flags
CC = mpicc
CFLAGS = -Wall -O3 -fopenmp

# Regras de Construção

# regra padrão executada ao digitar apenas 'make'
all: $(TARGET)

# Como construir o executável:
# cria o diretório bin, compila o código apontando o -o para dentro da pasta bin
$(TARGET): $(SRC)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

# regra para limpar os arquivos compilados
clean:
	rm -rf $(BIN_DIR)

# regra de atalho para rodar a simulação com 4 processos MPI
run: $(TARGET)
	mpirun --host localhost:4 ./$(TARGET)