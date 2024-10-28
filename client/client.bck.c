#include "header.h"

struct sockaddr_in server_address;
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

void spedisci_file(char *server_filename, int client_socket){
    // --- Spedisco il file ---
    FILE *f = fopen(server_filename, "r");
    if(f == NULL) error_("[CLIENT] Errore apertura file");
    // Salvo un int a 32 bit all'inizio del messaggio (come fosse un'header) per specificare la lunghezza del messaggio che sto inviando

    // Buffer per inviare il messaggio
    size_t headerSize = sizeof(uint32_t) + sizeof(char);
    size_t bufferSize = headerSize + sizeof(char) * MAX_MSG_CLUSTER;
    char *buffer = malloc(bufferSize);

    // Puntatore alla lunghezza del messaggio del buffer
    uint32_t *msgLenHeader = (uint32_t *) buffer;

    // Puntatore al flag per capire se il messaggio inviato è l'ultimo o meno
    char *msgFinHeader = (char *) (msgLenHeader + 1);

    // Puntatore al messaggio contenuto nel buffer
    char *msgBufferStart = msgFinHeader + 1;

    uint32_t r = 0;
    do {
        memset(buffer, 0, bufferSize);
        r = fread(msgBufferStart, sizeof(char), MAX_MSG_CLUSTER, f);
        *msgLenHeader = htonl(r);
        *msgFinHeader = feof(f) ? 1 : 0;
        sendto(client_socket, buffer, bufferSize, MSG_CONFIRM, (struct sockaddr *)&server_address, sizeof(server_address));
    } while(!feof(f));
    printf("[CLIENT] Download of %s finished\n", server_filename); 
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

void put(char *client_filename, int client_socket){
    int noFile;

    if(file_presente_nel_client(client_filename, client_socket) == 1){
        //Dico al server che è presente il file nel client
        noFile = 0;
        sendto(client_socket, &noFile, sizeof(int), MSG_CONFIRM, (struct sockaddr *)&server_address, sizeof(server_address));
        // Invio del nome del file
        int length = strlen(client_filename);
        sendto(client_socket, &length, sizeof(int), MSG_CONFIRM, (struct sockaddr *)&server_address, sizeof(server_address));
        sendto(client_socket, client_filename, length, MSG_CONFIRM, (struct sockaddr *)&server_address, sizeof(server_address));

        spedisci_file(client_filename, client_socket);
    }else{
        printf("Il file %s non è presente nella cartella", client_filename);
        noFile = 1;
        //Dico al server che non è presente file
        sendto(client_socket, &noFile, sizeof(int), MSG_CONFIRM, (struct sockaddr *)&server_address, sizeof(server_address));
    }

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

    //Creiamo una socket, identificata da un descrittore integer
    int client_socket;
    client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); //Uso UDP perché sto ho messo SOCK_DGRAM
    if(client_socket < 0) {
        fprintf(stderr, "Errore durante la creazione della socket.\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    // Specifichiamo un indirizzo per il socket a cui ci dobbiamko collegare col client
    server_address.sin_family = AF_INET;                // Specifico il dominio
    server_address.sin_port = htons(SERVER_PORT);       // Specifico la porta
    server_address.sin_addr.s_addr = INADDR_ANY;        // Host id

    socklen_t server_addr_len = sizeof(server_address);

    if(argc > 1) error_("./prog.c\n");

    cmd_t azione;
    char risposta[100];
    char * client_filename = malloc(MAX_FILE_NAME_LENGTH * sizeof(char));
    len_server_address = sizeof(server_address);

    while(1) {
        printf("\n\n---------Benvenuto nel server-----------");
        printf("\n1. Upload file \n");
        printf("\n2. Lista file uploadati \n");
        printf("\n3. Download file \n");
        printf("\nInserisci la tua scelta: ");

        char scelta = getchar();
        while(getchar() != '\n');
        printf("----------------------------------------\n");

        switch(scelta){
            case '1':
                azione = CMD_UPLOAD;
                printf("[CLIENT] Sending %u\n", azione);
                sendto(client_socket, &azione, sizeof(azione), MSG_CONFIRM, (struct sockaddr *) &server_address, len_server_address);
                /*//Spedisco al server la scelta che ho fatto
                memset(azione, 0, sizeof(azione));
                strcpy(azione, "UPLOAD");
                sendto(client_socket, azione, sizeof(azione), MSG_CONFIRM, (struct sockaddr *)&server_address, sizeof(server_address));
                printf("%s\n", azione);

                memset(risposta, 0, sizeof(risposta));
                recvfrom(client_socket, risposta, sizeof(risposta), MSG_WAITALL, (struct sockaddr *)&server_address, &len_server_address);
                printf("%s\n", risposta);
                if(!strcmp(risposta, "START")){
                    //----------Azione trasferimento file (put)----------------
                    memset(client_filename, 0, MAX_FILE_NAME_LENGTH);
                    leggi_stringa("Inserire nome del file: ", client_filename);

                    printf("[CLIENT] Sending %s...\n", client_filename);

                    put(client_filename, client_socket);
                    //---------------------------------------------------------
                }*/
                break;
            case '2':
                //Spedisco al server la scelta che ho fatto
                memset(azione, 0, sizeof(azione));
                strcpy(azione, "LIST");
                int length = strlen(client_filename);
                sendto(client_socket, azione, sizeof(azione), MSG_CONFIRM, (struct sockaddr *)&server_address, sizeof(server_address));
                printf("%s\n", azione);

                recvfrom(client_socket, risposta, sizeof(risposta), MSG_WAITALL, (struct sockaddr *)&server_address, &len_server_address);
                printf("%s\n", risposta);
                if(!strcmp(risposta, "START")){
                    list(client_socket);
                }
                break;
            case '3':
                //Spedisco al server la scelta che ho fatto
                memset(azione, 0, sizeof(azione));
                strcpy(azione, "DOWNLOAD");
                sendto(client_socket, azione, sizeof(azione), MSG_CONFIRM, (struct sockaddr *)&server_address, sizeof(server_address));
                printf("%s\n", azione);

                recvfrom(client_socket, risposta, sizeof(risposta), MSG_WAITALL, (struct sockaddr *)&server_address, &len_server_address);
                printf("%s\n", risposta);

                if(!strcmp(risposta, "START")){
                    //Dico il file di cui voglio fare download
                    char *server_filename = malloc(MAX_FILE_NAME_LENGTH*sizeof(char));
                    memset(server_filename, 0, MAX_FILE_NAME_LENGTH);
                    leggi_stringa("Inserire nome del file: ", server_filename);
                    get(server_filename, client_socket);
                }
                break;
            
        }
                    
        
    }


    close(client_socket);
    return 0;
}