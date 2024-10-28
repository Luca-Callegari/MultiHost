#include "header.h"

struct sockaddr_in server_address;
socklen_t server_addr_len;
int len_server_address;

void error_(char *prompt){
    fprintf(stderr, "%s", prompt);
    fflush(stderr);
}

void errore_apertura_file(FILE *f){
    if (f == NULL) {
        perror("Errore nell'apertura del file");
        return;
    }
}

void leggi_stringa(char *prompt, char *input){
    memset(input, 0, MAX_FILE_NAME_LENGTH);
    printf("%s", prompt);
    if(fgets(input, MAX_FILE_NAME_LENGTH, stdin) != NULL){
        int len = strlen(input);
        if(len > 0 && input[len - 1] == '\n'){
            input[len - 1] = '\0';  
        }
    }else{
        printf("Errore!");
    }
}

int file_presente_nel_client(char *client_filename, int client_socket){
    struct dirent *de;  // Struttura per contenere le informazioni sui file

    // Aprire la directory corrente "."
    DIR *dr = opendir(".");

    if (dr == NULL) {  // Se la directory non può essere aperta
        perror("Errore nell'apertura della directory");
    }

    // Leggere e stampare i file presenti nella directory
    //printf("File presenti nella cartella corrente:\n");
    while ((de = readdir(dr)) != NULL) {
        //send(client_socket, de->d_name, sizeof(de->d_name), 0);
        //printf("%s\n", de->d_name);  // Stampa il nome del file/directory
        if(!strcmp(de->d_name, client_filename)){
            // Chiudere la directory
            closedir(dr);
            return 1;
        }
    }
    // Chiudere la directory
    closedir(dr);
    return 0;
}

packet_t* make_packet(cmd_t _cmd, pos_t _pos, char _fin_flag, char *_filename, packet_len_t _filename_len, void *_body, packet_len_t _body_len) {

    packet_len_t packet_len = sizeof(packet_header_t) + _body_len + _filename_len;

    // Alloca il pacchetto
    packet_t *packet = malloc(packet_len);
    if (!packet) {
        perror("[CLIENT] Errore durante l'allocazione del pacchetto");
        return NULL;
    }

    char *filename_ptr = (char *) ((char *) packet + sizeof(packet_header_t));
    char *body_ptr = filename_ptr + _filename_len;

    packet->header.cmd = htons(_cmd);
    packet->header.len = htonl(packet_len);
    packet->header.body_len = htonl(_body_len);
    packet->header.pos = htonl(_pos);
    packet->header.fin_flag = _fin_flag;
    memcpy(filename_ptr, _filename, _filename_len);
    memcpy(body_ptr, _body, _body_len);
    return packet;

}

void spedisci_file(char *server_filename, int client_socket){

    // --- Spedisco il file ---
    FILE *f = fopen(server_filename, "r");
    packet_len_t filename_len = strlen(server_filename) + 1;
    if(f == NULL) error_("[CLIENT] Errore apertura file");
    
    // Buffer per inviare il messaggio
    void *buffer = malloc(MAX_MSG_CLUSTER);

    uint32_t r = 0;
    packet_t *packet;
    pos_t pos = 0;
    char fin_flag;
    do {
        memset(buffer, 0, MAX_MSG_CLUSTER);
        pos = ftell(f);
        r = fread(buffer, sizeof(char), MAX_MSG_CLUSTER, f);
        fin_flag = feof(f) ? 1 : 0;
        packet = make_packet(CMD_UPLOAD, pos, fin_flag, server_filename, filename_len, buffer, r);
        printf("[CLIENT] Inviando un pacchetto di %d bytes (%d)\n", ntohl(packet->header.len), ntohl(packet->header.body_len));
        sendto(client_socket, packet, ntohl(packet->header.len), MSG_CONFIRM, (struct sockaddr *) &server_address, server_addr_len);
        //sleep(1);
        free(packet);
    } while(!feof(f));
    printf("\n[CLIENT] Upload of %s finished\n", server_filename); 
    fclose(f);
    free(buffer);

}

void ricevi_file(char *client_filename, int client_socket){
    // --- Salvo il file ---

    FILE *f = fopen(client_filename, "w");
    if(f == NULL) error_("[CLIENT] Errore apertura file");

    // Buffer per ricevere il messaggio
    size_t headerSize = sizeof(uint32_t) + sizeof(char);
    size_t bufferSize = headerSize + sizeof(char) * MAX_MSG_CLUSTER;
    char *buffer = malloc(bufferSize);

    // Puntatore alla lunghezza del messaggio del buffer
    uint32_t *msgLenHeader = (uint32_t *) buffer;

    // Puntatore al flag per capire se il messaggio inviato è l'ultimo o meno
    char *msgFinHeader = (char *) (msgLenHeader + 1);

    // Puntatore al messaggio contenuto nel buffer
    char *msgBufferStart = msgFinHeader + 1;

    uint32_t msgLen = 0;
    char msgFin = 0;
    do {
        memset(buffer, 0, bufferSize);
        recvfrom(client_socket, buffer, bufferSize, MSG_WAITALL, (struct sockaddr *)&server_address, &len_server_address);
        msgLen = ntohl(*msgLenHeader);
        msgFin = *msgFinHeader;
        fwrite(msgBufferStart, sizeof(char), msgLen, f);
    } while(!msgFin);
    printf("[CLIENT] Upload of %s finished\n", client_filename); 
    fclose(f);
    free(buffer);
}

void get(char *server_filename, int client_socket){

    int length = strlen(server_filename);
    sendto(client_socket, &length, sizeof(int), MSG_CONFIRM, (struct sockaddr *)&server_address, sizeof(server_address));
    sendto(client_socket, server_filename, length, MSG_CONFIRM, (struct sockaddr *)&server_address, sizeof(server_address));
    printf("%s", server_filename);

    int noFile;
    int length_ = sizeof(server_address);
    recvfrom(client_socket, &noFile, sizeof(int), MSG_TRUNC, (struct sockaddr *)&server_address, &length);

    if(noFile == 1){
        //Creo file destinazione nel client
        FILE *f = fopen(server_filename, "w+");
        if(f == NULL) error_("Errore apertura file\n");
        ricevi_file(server_filename, client_socket);
    }else{
        printf("File non presente nel server!");
    }

}

void list(int client_socket){
    //Prendo il numero di righe del file dal server (caricati da client)
    char str[256];
    recvfrom(client_socket, str, sizeof(str), MSG_WAITALL, (struct sockaddr *)&server_address, &len_server_address);
    int righe = (int)strtoul(str, NULL, 10);
    printf("Il numero di file presenti nel server sono: %d", righe);
    //Prendo tutte le stringhe (riga per riga) che sono presenti nel filename.txt nel server
    printf("\nI file presenti nel server (caricati dal client) sono: \n");
    char buffer[MAX_FILE_NAME_LENGTH];
    memset(buffer, 0, MAX_FILE_NAME_LENGTH);
    for(int i = 0; i < righe; i++){
        recvfrom(client_socket, buffer, sizeof(buffer), MSG_WAITALL, (struct sockaddr *)&server_address, &len_server_address);
        printf("%s\n", buffer);
    }
}

int main(int argc, char **argv){

    // Creiamo una socket, identificata da un descrittore integer
    int client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(client_socket < 0) {
        fprintf(stderr, "[CLIENT] Errore durante la creazione della socket.\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    // Specifichiamo un indirizzo per il socket a cui ci dobbiamko collegare col client
    server_address.sin_family = AF_INET;                // Specifico il dominio
    server_address.sin_port = htons(SERVER_PORT);       // Specifico la porta
    server_address.sin_addr.s_addr = INADDR_ANY;        // Host id

    server_addr_len = sizeof(server_address);
    
    cmd_t cmd;
    packet_t *packet;
    char client_filename[MAX_FILE_NAME_LENGTH];

    printf("\n\n\tBenvenuto nel server\n\n\n\n");

    while(1) {

        printf("\n---------Menu-----------\n\n");
        printf("\t1. Upload file \n");
        printf("\t2. Lista file uploadati \n");
        printf("\t3. Download file\n");
        printf("\n----------------------------------------\n");

        printf("\nInserisci la tua scelta: ");
        char scelta = getchar();
        while(getchar() != '\n');

        switch(scelta){
            case '1':
                leggi_stringa("Inserire nome del file: ", client_filename);
                printf("[CLIENT] Sending %s...\n", client_filename);
                fflush(stdout);
                if(file_presente_nel_client(client_filename, client_socket)) {
                    spedisci_file(client_filename, client_socket);
                } else {
                    fprintf(stderr, "[CLIENT] File %s non presente\n", client_filename);
                    fflush(stderr);
                }
                break;
            case '2':
                break;
            case '3':
                break;
            
        }
        
        
    }

    close(client_socket);
    exit(EXIT_SUCCESS);

}