#define SERVER_PORT 8080
#define CLIENT_PORT 8081
#define MAX_FILE_NAME_LENGTH 4096
#define MAX_MSG_CLUSTER 256
#define MAX_PACKET_BODY_LEN 2048
#define MAX_PACKET_LEN sizeof(packet_header_t) + MAX_PACKET_BODY_LEN * sizeof(char)
#define QUEUE_SIZE 100
#define WORKER_THREADS_NUM 5