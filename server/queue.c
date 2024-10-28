#pragma once
#include "header.h"

// Inizializzazione della coda di pacchetti
void init_queue(packet_queue_t *q) {
    q->front = 0;
    q->rear = 0;
    q->count = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond_not_empty, NULL);
    pthread_cond_init(&q->cond_not_full, NULL);
}

// Funzione per inserire un pacchetto nella coda (enqueue)
void enqueue(packet_queue_t *q, packet_t *packet) {
    pthread_mutex_lock(&q->mutex);
    
    // Attendere se la coda è piena
    while (q->count == QUEUE_SIZE) {
        pthread_cond_wait(&q->cond_not_full, &q->mutex);
    }

    q->packets[q->rear] = packet;
    q->rear = (q->rear + 1) % QUEUE_SIZE;
    q->count++;

    // Segnalare che la coda non è più vuota
    pthread_cond_signal(&q->cond_not_empty);

    pthread_mutex_unlock(&q->mutex);
}

// Funzione per rimuovere un pacchetto dalla coda (dequeue)
packet_t *dequeue(packet_queue_t *q) {
    pthread_mutex_lock(&q->mutex);

    // Attendere se la coda è vuota
    while (q->count == 0) {
        pthread_cond_wait(&q->cond_not_empty, &q->mutex);
    }

    packet_t *packet = q->packets[q->front];
    q->front = (q->front + 1) % QUEUE_SIZE;
    q->count--;

    // Segnalare che la coda non è più piena
    pthread_cond_signal(&q->cond_not_full);

    pthread_mutex_unlock(&q->mutex);

    return packet;
}