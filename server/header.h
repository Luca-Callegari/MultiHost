#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>

#include "../shared/config.h"
#include "../shared/packet.h"

typedef struct {
    char *ip;
    int port;
} client_id_t;

typedef struct {
    packet_t *packets[QUEUE_SIZE];
    int front;
    int rear;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t cond_not_empty;
    pthread_cond_t cond_not_full;
} packet_queue_t;

typedef struct {
    packet_queue_t** queues;
    int size;
    pthread_mutex_t hash_mutex;
} hashtable_t;