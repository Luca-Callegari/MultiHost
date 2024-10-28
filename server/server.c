#include "header.h"
#include "queue.c"

int server_socket;
struct sockaddr_in server_address;
int len_server_address;
int contatore_file_server = 0;

void error_(char *prompt){
    fprintf(stderr, "%s", prompt);
    fflush(stderr);
}

void save_filename_in_filename_txt(char *client_filename){
    //Salvo il client_filename in files[]
    /*files[index_filename] = malloc(MAX_FILE_NAME_LENGTH*sizeof(char));
    strcpy(files[index_filename], client_filename);
    printf("%s\n", files[index_filename]);
    index_filename++;
    */

    //Scrivo i nomi dei file che sto creando su un file chiamato filename.txt
    FILE *f = fopen("filename.txt", "a");
    if(f == NULL) error_("Errore nella scrittura di in un file");
    fprintf(f,"%s\n", client_filename);
    fseek(f, 0 , SEEK_END);
}

void spedisci_file(char *server_filename, int client_socket){
    // --- Spedisco il file ---
    FILE *f = fopen(server_filename, "r");
    if(f == NULL) error_("[SERVER] Errore apertura file");
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
    printf("[SERVER] Download of %s finished\n", server_filename); 
    fclose(f);
    free(buffer);
}

void ricevi_file(packet_t *packet){
    
    pthread_t tid = pthread_self();

    // --- Salvo il file ---

    char *filename = (char *) ((char *) packet + sizeof(packet_header_t));
    char *body = filename + strlen(filename) + 1;

    printf("[THREAD-%lu] Uploading %s at %d\n", tid, filename, packet->header.pos);
    fflush(stdout);

    FILE *f = NULL;
    if(packet->header.pos == 0) {
        save_filename_in_filename_txt(filename);
        f = fopen(filename, "w");
        if(f == NULL) error_("[SERVER] Errore apertura file (1)");
    } else {
        f = fopen(filename, "r+");
        if(f == NULL) error_("[SERVER] Errore apertura file (2)");
        if (fseek(f, (long) packet->header.pos, SEEK_SET) != 0) error_("[SERVER] Errore nel posizionamento del cursore");
    }
    if(f == NULL) error_("[SERVER] Errore apertura file (3)");

    fwrite(body, sizeof(char), packet->header.body_len, f);
    fflush(f);
    if(packet->header.fin_flag) printf("[SERVER] Upload of %s finished\n", filename); 
    fclose(f);
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

int file_presente_nel_server(char *server_filename, int client_socket){
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
        if(!strcmp(de->d_name, server_filename)){
            // Chiudere la directory
            closedir(dr);
            return 1;
        }
    }
    // Chiudere la directory
    closedir(dr);
    return 0;
}

void get(char *server_filename, int client_socket){
    //File che devo travasare e travaso
    int noFile;
    if(file_presente_nel_server(server_filename, client_socket) == 1){
        noFile = 1;
        sendto(client_socket, &noFile, sizeof(int), MSG_CONFIRM, (struct sockaddr *)&server_address, sizeof(server_address));
        FILE *f = fopen(server_filename, "r");
        errore_apertura_file(f);
        printf("%s", server_filename);
        spedisci_file(server_filename, client_socket);
    }else{
        noFile = 0;
        sendto(client_socket, &noFile, sizeof(int), MSG_CONFIRM, (struct sockaddr *)&server_address, sizeof(server_address));
    }
        
}

void recezione(int client_socket){

    int length;
    recvfrom(client_socket, &length, sizeof(int), MSG_WAITALL, (struct sockaddr *) &server_address, &len_server_address);

    char *server_filename = malloc(length + 1);  // Alloca spazio per la stringa e il terminatore
    memset(server_filename, 0, length + 1);
    recvfrom(client_socket, server_filename, length, MSG_WAITALL, (struct sockaddr *)&server_address, &len_server_address);
    server_filename[length] = '\0';  // Termina la stringa

    get(server_filename, client_socket);
}



void list(int client_socket){
    //DEVO PRENDERE RIGA PER RIGA DA FILENAME.TXT E MANDARLO AL CLIENT
    FILE *f = fopen("filename.txt", "r");
    errore_apertura_file(f);
    fseek(f,0,SEEK_SET);
    char buffer[MAX_FILE_NAME_LENGTH];
    while(fscanf(f, "%s", buffer) != EOF){
        fprintf(stdout, "%s \n", buffer);
        sendto(client_socket, buffer, sizeof(buffer), MSG_CONFIRM, (struct sockaddr *)&server_address, sizeof(server_address));
    }
    printf("Trasferimento nomi file uploadati completato con successo!\n");
    fclose(f);
}

int n_righe_file(FILE *f){
    char buffer[MAX_FILE_NAME_LENGTH];
    int count_righe = 0;
    while(fscanf(f, "%s", buffer) != EOF){
        count_righe++;
    }
    return count_righe;
}

void* worker_thread(void* args) {

    pthread_t tid = pthread_self();

    // Coda per la gestione dei pacchetti in arrivo
    packet_queue_t* q = (packet_queue_t*) args;

    // Stampa l'indirizzo IP e la porta del client
    char *client_ip;
    int client_port;

    packet_t *packet;

    while(1) {

        packet = dequeue(q);

        // Stampa l'indirizzo IP e la porta del client
        client_ip = inet_ntoa(packet->header.address.sin_addr);
        client_port = ntohs(packet->header.address.sin_port);

        printf("[THREAD-%lu] Indirizzo IP del client: %s\n", tid, client_ip);
        printf("[THREAD-%lu] Porta del client: %d\n", tid, client_port);

        // Stampa il comando cmd
        printf("[THREAD-%lu] Comando da elaborare: %u\n", tid, packet->header.cmd);

        // Processa il comando
        switch (packet->header.cmd) {
            case CMD_UPLOAD:
                ricevi_file(packet);
                break;
            case CMD_LIST:
                break;
            case CMD_DOWNLOAD:
                recezione(server_socket);
                break;
            default:
                fprintf(stderr, "[THREAD-%lu] Comando non riconosciuto: %d\n", tid, packet->header.cmd);
                fflush(stderr);
                break;
        }

        free(packet);

    }

    /*
    //Creiamo una socket, identificata da un descrittore integer
    int client_socket;
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);

    server_address.sin_family = AF_INET; //Specifico il dominio
    server_address.sin_port = htons(PORT); //Specifico la porta
    server_address.sin_addr.s_addr = INADDR_ANY;

    //collego il server con il client
    bind(client_socket,(struct sockaddr*) &server_address, sizeof(server_address));

    char azione[100];
    char risposta[] = "START";
    len_server_address = sizeof(server_address);

    while(1) {
        memset(azione, 0, sizeof(azione));
        recvfrom(client_socket, azione, sizeof(azione), MSG_WAITALL, (struct sockaddr *)&server_address, &len_server_address);
        printf("%s\n", azione);
        if(strcmp(azione, "UPLOAD") == 0){
            sendto(client_socket, risposta, sizeof(risposta), MSG_CONFIRM, (struct sockaddr *)&server_address, sizeof(server_address));
            put(client_socket);
        }else if(strcmp(azione, "LIST") == 0){
            sendto(client_socket, risposta, sizeof(risposta), MSG_CONFIRM, (struct sockaddr *)&server_address, sizeof(server_address));

            //Calcolo il numero di righe presenti in file.txt, da mandare al client
            FILE *f = fopen("filename.txt", "r");
            errore_apertura_file(f);
            int righe = n_righe_file(f);
            char str[10];  // Buffer per la stringa
            sprintf(str, "%d", righe);  // Converte l'intero in stringa
            sendto(client_socket, str, sizeof(str), MSG_CONFIRM, (struct sockaddr *)&server_address, sizeof(server_address));

            //Effettuo trasferimento filename.txt
            puts("\n-------FILE PRESENTI NEL SERVER--------");
            list(client_socket); 
            //file_presenti_nel_server(client_socket);
        }else if(strcmp(azione, "DOWNLOAD") == 0){
            sendto(client_socket, risposta, sizeof(risposta), MSG_CONFIRM, (struct sockaddr *)&server_address, sizeof(server_address));
            recezione(client_socket);
        }
    
    }

    close(client_socket);
    return 0;

    */

    pthread_exit(NULL);

}

unsigned long hash(const char *str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

int main(int argc, char**argv) {

    server_address.sin_family = AF_INET;            // Dominio della socket
    server_address.sin_port = htons(SERVER_PORT);   // Porta da utilizzare
    server_address.sin_addr.s_addr = INADDR_ANY;    // Indirizzo su cui ascolta la socket
    
    // Socket per gestire le richieste in arrivo dai client
    server_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(server_socket < 0) {
        fprintf(stderr, "[MAIN] Errore durante la creazione della socket.\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        fprintf(stderr, "[MAIN] Errore durante l'impostazione di SO_REUSEADDR.\n");
        exit(EXIT_FAILURE);
    }

    // Associo la socket ai metadati definiti
    if (bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        fprintf(stderr, "[MAIN] Errore durante il bind.\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    // --- CREAZIONE WORKER THREAD ---

    packet_queue_t packet_queues[WORKER_THREADS_NUM];
    pthread_t thread[WORKER_THREADS_NUM];

    for(int i=0; i<WORKER_THREADS_NUM; i++) {
        
        init_queue(&(packet_queues[i]));

        // Creazione del thread
        if (pthread_create(&(thread[i]), NULL, worker_thread, (void *) &(packet_queues[i]))) {
            fprintf(stderr, "Errore nella creazione del thread.\n");
            fflush(stderr);
            exit(EXIT_FAILURE);
        }
    }

    // --- GESTIONE PACCHETTI IN INGRESSO ---

    struct sockaddr_in client_address;
    socklen_t client_addr_len = sizeof(client_address);
    packet_t *packet;

    char *client_ip;
    int client_port;
    int index;

    while(1) {

        packet = malloc(MAX_PACKET_LEN);

        // Attendo richieste da parte dei client
        recvfrom(server_socket, packet, MAX_PACKET_LEN, 0, (struct sockaddr *) &client_address, &client_addr_len);

        //mi salvo l'indirizzo client nella header del pacchetto
        memcpy(&(packet->header.address), &client_address, client_addr_len);
        packet->header.len = ntohl(packet->header.len);
        packet->header.body_len = ntohl(packet->header.body_len);
        packet->header.cmd = ntohs(packet->header.cmd);
        packet->header.pos = ntohl(packet->header.pos);

        char *filename = (char *) ((char *) packet + sizeof(packet_header_t));

        client_ip = inet_ntoa(packet->header.address.sin_addr);
        client_port = ntohs(packet->header.address.sin_port);

        printf("[MAIN] Pacchetto ricevuto - indirizzo IP del client: %s\n", client_ip);
        printf("[MAIN] Pacchetto ricevuto - porta del client: %d\n", client_port);

        printf("[MAIN] Comando ricevuto (cmd): %u\n", packet->header.cmd);

        printf("[MAIN] Hashing di: %s\n", filename);
        fflush(stdout);
        index = hash(filename) % WORKER_THREADS_NUM;
        printf("[MAIN] Asseganto al thread %d\n", index);
        fflush(stdout);
        enqueue(&(packet_queues[index]), packet);

    }

    close(server_socket);
    return EXIT_SUCCESS;

}