# Nome dei compilatori e delle opzioni
CC = gcc
CFLAGS = -g -pthread

# Cartelle
CLIENT_DIR = ./client
SERVER_DIR = ./server

# File sorgente
CLIENT_SRC = $(CLIENT_DIR)/client.c
SERVER_SRC = $(SERVER_DIR)/server.c

# File eseguibili
CLIENT_EXE = $(CLIENT_DIR)/client
SERVER_EXE = $(SERVER_DIR)/server

# Regola predefinita
all: $(CLIENT_EXE) $(SERVER_EXE)

# Compilazione client
$(CLIENT_EXE): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT_EXE) $(CLIENT_SRC)

# Compilazione server
$(SERVER_EXE): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER_EXE) $(SERVER_SRC)

# Pulizia dei file compilati
clean:
	rm -f $(CLIENT_EXE) $(SERVER_EXE)

# Regola per pulire solo il client
clean_client:
	rm -f $(CLIENT_EXE)

# Regola per pulire solo il server
clean_server:
	rm -f $(SERVER_EXE)

.PHONY: all clean clean_client clean_server
