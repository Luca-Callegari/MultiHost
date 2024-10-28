#pragma once

#include <stdint.h>

#define CMD_UPLOAD 1
#define CMD_DOWNLOAD 2
#define CMD_LIST 3

typedef uint16_t packet_type_t;
typedef uint32_t packet_len_t;
typedef uint16_t cmd_t;
typedef uint32_t pos_t;

typedef struct _packet_header_t {
    packet_len_t len;
    packet_len_t body_len;
    cmd_t cmd;
    pos_t pos;
    char fin_flag;
    struct sockaddr_in address;
} packet_header_t;

typedef struct _packet_t {
    packet_header_t header;
    void *filename;
    void *body;
} packet_t;