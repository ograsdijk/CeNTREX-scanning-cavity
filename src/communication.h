#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include "data_structs.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "config.h"

typedef struct{
    struct sockaddr_in address;
    int port;
    int socket;
    int server_fd;
} socket_connection_t;

typedef enum {
    READ_CH1,
    READ_CH2,
    READ_FILTERED1,
    READ_FILTERED2,
    READ_FREQUENCY,
    WRITE_FREQUENCY,
    READ_AMPLITUDE,
    WRITE_AMPLITUDE,
    READ_OFFSET,
    WRITE_OFFSET,
    READ_NPEAKS1,
    READ_NPEAKS2,
    READ_PEAKLOCS1,
    READ_PEAKLOCS2,
    READ_PEAKLOCSF1,
    READ_PEAKLOCSF2,
    READ_RUNNING,
    WRITE_RUNNING,
    READ_RUN_MASTER,
    WRITE_RUN_MASTER,
    READ_RUN_SLAVE,
    WRITE_RUN_SLAVE,
    WRITE_SETPOINT_MASTER,
    WRITE_SETPOINT_SLAVE,
    WRITE_P_MASTER,
    WRITE_P_SLAVE,
    WRITE_I_MASTER,
    WRITE_I_SLAVE,
    WRITE_D_MASTER,
    WRITE_D_SLAVE,
    WRITE_MIN_MASTER,
    WRITE_MIN_SLAVE,
    WRITE_MAX_MASTER,
    WRITE_MAX_SLAVE,
    WRITE_MAXCHANGE_MASTER,
    WRITE_MAXCHANGE_SLAVE,
    WRITE_MAXSTD_MASTER,
    WRITE_MAXSTD_SLAVE,
    READ_MASTER,
    READ_SLAVE,
    READ_ALL
} request_t;

int setup_socket_server(socket_connection_t *connection);
int wait_for_connection(socket_connection_t *connection);
int write_socket_array_float(int sock, float array[], int32_t array_size);
int write_socket_array_int16(int sock, int16_t array[], int32_t array_size);
int write_socket_int32(int sock, int32_t value);
int read_socket_int32(int sock, int32_t *value);
int write_socket_char(int sock, char value);
int read_socket_float(int sock, float *value);
int write_socket_float(int sock, float value);
int handle_requests(socket_connection_t *connection, request_t request, cavity_data_t *data);
void polling(socket_connection_t *connection, cavity_data_t *data);
void *do_socket_communication(void *args);
int copy_settings_network_to_local(cavity_data_t *network, cavity_data_t *local);
int copy_local_to_network(cavity_data_t *local, cavity_data_t *network);
void copy_lock_settings(lock_t *dest, lock_t *src);
int write_socket_bytes(int sock, void *p, int32_t nbytes);
int read_socket_bytes(int sock, void *p, int32_t nbytes);
#endif