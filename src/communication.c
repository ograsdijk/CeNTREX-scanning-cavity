#include "communication.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int setup_socket_server(socket_connection_t *connection){
    int *server_fd = &(connection->server_fd);
    struct sockaddr_in *address = &(connection->address);
    int opt = 1;
    int port = connection->port;

    // Creating socket file descriptor
    if ((*server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        // exit(EXIT_FAILURE);
    }
    
    // Forcefully attaching socket to the port 8080
    if (setsockopt(*server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                            &opt, sizeof(opt)))
    {
        perror("setsockopt");
        // exit(EXIT_FAILURE);
    }
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = INADDR_ANY;
    address->sin_port = htons( port );
    
    // Forcefully attaching socket to the port 8080
    if (bind(*server_fd, (struct sockaddr *)address, sizeof(*address))<0)
    {
        perror("bind failed");
        // exit(EXIT_FAILURE);
    }
    if (listen(*server_fd, 3) < 0)
    {
        perror("listen");
        // exit(EXIT_FAILURE);
    }
    return 0;
}

int wait_for_connection(socket_connection_t *connection){
    int *server_fd = &(connection->server_fd);
    struct sockaddr_in *address = &(connection->address);
    int *sock = &(connection->socket);
    int addrlen = sizeof(*address);
    if ((*sock = accept(*server_fd, (struct sockaddr *)address, 
                                                    (socklen_t*)&addrlen))<0)
    {
        perror("accept");
        // exit(EXIT_FAILURE);
    }
    return 0;
}

int write_socket_array_float(int sock, float array[], int32_t array_size)
{   
    ssize_t write_pointer = 0;
    do {
        write_pointer += write(sock, &array_size+write_pointer, sizeof(array_size) - write_pointer);
    } while (write_pointer != sizeof(array_size));
    write_pointer = 0;
    do {
        write_pointer += write(sock, array+write_pointer, sizeof(float)*(array_size) - write_pointer);
    } while (write_pointer != sizeof(float)*(array_size));
    return write_pointer;
}


int write_socket_array_int16(int sock, int16_t array[], int32_t array_size)
{   
    ssize_t write_pointer = 0;
    do {
        write_pointer += write(sock, &array_size+write_pointer, sizeof(array_size) - write_pointer);
    } while (write_pointer != sizeof(array_size));
    write_pointer = 0;
    do {
        write_pointer += write(sock, array+write_pointer, sizeof(int16_t)*(array_size) - write_pointer);
    } while (write_pointer != sizeof(int16_t)*(array_size));
    return write_pointer;
}

int write_socket_int32(int sock, int32_t value){
    ssize_t write_pointer = 0;
    do {
        write_pointer += write(sock, &value + write_pointer, sizeof(value) - write_pointer);
    } while(write_pointer != sizeof(value));
    return write_pointer;
}

int read_socket_int32(int sock, int32_t *value){
    ssize_t read_pointer = 0;
    do{
        read_pointer += read(sock, value + read_pointer, sizeof(*value) - read_pointer);
    } while(read_pointer != sizeof(*value));
    return read_pointer;
}

int write_socket_char(int sock, char value){
    ssize_t write_pointer = 0;
    do {
        write_pointer += write(sock, &value + write_pointer, sizeof(value) - write_pointer);
    } while(write_pointer != sizeof(value));
    return write_pointer;
}

int read_socket_char(int sock, char *value){
    ssize_t read_pointer = 0;
    do{
        read_pointer += read(sock, value + read_pointer, sizeof(*value) - read_pointer);
    } while(read_pointer != sizeof(*value));
    return read_pointer;
}

int read_socket_float(int sock, float *value){
    ssize_t read_pointer = 0;
    do{
        read_pointer += read(sock, value + read_pointer, sizeof(*value) - read_pointer);
    } while(read_pointer != sizeof(*value));
    return read_pointer;
}

int write_socket_float(int sock, float value){
    ssize_t write_pointer = 0;
    do {
        write_pointer += write(sock, &value + write_pointer, sizeof(value) - write_pointer);
    } while(write_pointer != sizeof(value));
    return write_pointer;
}

int write_socket_bytes(int sock, void *p, int32_t nbytes){
    ssize_t write_pointer = 0;
    do {
        write_pointer += write(sock, p + write_pointer, nbytes - write_pointer);
    } while(write_pointer != nbytes);
    return write_pointer;
}

int read_socket_bytes(int sock, void *p, int32_t nbytes){
    ssize_t read_pointer = 0;
    do {
        read_pointer += read(sock, p + read_pointer, nbytes - read_pointer);
    } while(read_pointer != nbytes);
    return read_pointer;
}

int handle_requests(socket_connection_t *connection, request_t request, cavity_data_t *data){
    int sock = connection->socket;
    int npeaks;
    char *running;
    lock_t *lock;

    // printf("received request %d\n", request);

    pthread_mutex_lock(&mutex);
    switch(request){
        case READ_CH1:
            write_socket_array_int16(connection->socket, data->ch1, BUFF_SIZE);
            break;
        case READ_CH2:
            write_socket_array_int16(connection->socket, data->ch2, BUFF_SIZE);
            break;
        case READ_FILTERED1:
            write_socket_array_float(connection->socket, data->filtered1, BUFF_SIZE);
            break;
        case READ_FILTERED2:
            write_socket_array_float(connection->socket, data->filtered2, BUFF_SIZE);
            break;
        case READ_FREQUENCY:
            write_socket_float(sock, data->frequency);
            break;
        case WRITE_FREQUENCY:
            read_socket_float(connection->socket, &(data->frequency));
            data->changed_config = 1;
            data->changed_awg = 1;
            break;
        case READ_AMPLITUDE:
            write(connection->socket, &(data->amplitude), sizeof(data->amplitude));
            break;
        case WRITE_AMPLITUDE:
            read_socket_float(connection->socket, &(data->amplitude));
            data->changed_config = 1;
            data->changed_awg = 1;
            break;
        case READ_OFFSET:
            write(connection->socket, &(data->offset), sizeof(data->offset));
            break;
        case WRITE_OFFSET:
            read_socket_float(connection->socket, &(data->offset));
            data->changed_config = 1;
            data->changed_awg = 1;
            break;
        case READ_NPEAKS1:
            npeaks = ((data->peaks_ch1).npeaks);
            write_socket_int32(connection->socket, npeaks);
            break;
        case READ_NPEAKS2:
            npeaks = ((data->peaks_ch2).npeaks);
            write_socket_int32(connection->socket, npeaks);
            break;
        case READ_PEAKLOCS1:
            npeaks = ((data->peaks_ch1).npeaks);
            write_socket_array_int16(connection->socket, (data->peaks_ch1).locs, npeaks);
            break;
        case READ_PEAKLOCS2:
            npeaks = (((data->peaks_ch2)).npeaks);
            write_socket_array_int16(connection->socket, (data->peaks_ch1).locs, npeaks);
            break;
        case READ_PEAKLOCSF1:
            npeaks = (((data->peaks_ch1)).npeaks);
            write_socket_array_float(connection->socket, (data->peaks_ch1).locs_f, npeaks);
            break;
        case READ_PEAKLOCSF2:
            npeaks = (((data->peaks_ch2)).npeaks);
            write_socket_array_float(connection->socket, (data->peaks_ch1).locs_f, npeaks);
            break;
        case READ_RUNNING:
            running = &(data->running);
            write_socket_char(connection->socket, *running);
            break;
        case WRITE_RUNNING:
            running = &(data->running);
            read_socket_char(connection->socket, running);
            data->changed_config = 1;
            break;
        case READ_RUN_MASTER:
            running = &(data->run_master);
            write_socket_char(connection->socket, *running);
            break;
        case WRITE_RUN_MASTER:
            running = &(data->run_master);
            read_socket_char(connection->socket, running);
            data->changed_config = 1;
            break;
        case READ_RUN_SLAVE:
            running = &(data->run_slave);
            write_socket_char(connection->socket, *running);
            break;
        case WRITE_RUN_SLAVE:
            running = &(data->run_slave);
            read_socket_char(connection->socket, running);
            data->changed_config = 1;
            break;
        case WRITE_SETPOINT_MASTER:
            lock = &(data->master);
            read_socket_float(sock, &(lock->setpoint));
            break;
        case WRITE_SETPOINT_SLAVE:
            lock = &(data->slave);
            read_socket_float(sock, &(lock->setpoint));
            break;
        case WRITE_P_MASTER:
            lock = &(data->master);
            read_socket_float(sock, &(lock->P));
            break;
        case WRITE_P_SLAVE:
            lock = &(data->slave);
            read_socket_float(sock, &(lock->P));
            break;
        case WRITE_I_MASTER:
            lock = &(data->master);
            read_socket_float(sock, &(lock->I));
            break;
        case WRITE_I_SLAVE:
            lock = &(data->slave);
            read_socket_float(sock, &(lock->I));
            break;
        case WRITE_D_MASTER:
            lock = &(data->master);
            read_socket_float(sock, &(lock->D));
            break;
        case WRITE_D_SLAVE:
            lock = &(data->slave);
            read_socket_float(sock, &(lock->D));
            break;
        case WRITE_MIN_MASTER:
            lock = &(data->master);
            read_socket_float(sock, &(lock->min));
            break;
        case WRITE_MIN_SLAVE:
            lock = &(data->slave);
            read_socket_float(sock, &(lock->min));
            break;
        case WRITE_MAX_MASTER:
            lock = &(data->master);
            read_socket_float(sock, &(lock->max));
            break;
        case WRITE_MAX_SLAVE:
            lock = &(data->slave);
            read_socket_float(sock, &(lock->max));
            break;
        case WRITE_MAXCHANGE_MASTER:
            lock = &(data->master);
            read_socket_float(sock, &(lock->maxchange));
            break;
        case WRITE_MAXCHANGE_SLAVE:
            lock = &(data->slave);
            read_socket_float(sock, &(lock->maxchange));
            break;
        case WRITE_MAXSTD_MASTER:
            lock = &(data->master);
            read_socket_float(sock, &(lock->maxstd));
            break;
        case WRITE_MAXSTD_SLAVE:
            lock = &(data->slave);
            read_socket_float(sock, &(lock->maxstd));
            break;
        case READ_MASTER:
            lock = &(data->master);
            write_socket_bytes(sock, lock, sizeof(lock_t));
            break;
        case READ_SLAVE:
            lock = &(data->slave);
            write_socket_bytes(sock, lock, sizeof(lock_t));
            break;
        case READ_ALL:
            write_socket_bytes(sock, data, sizeof(cavity_data_t));
            break;
        default:
            printf("%d not yet implemented\n", request);
    }
    pthread_mutex_unlock(&mutex);
    return 0;
}

void polling(socket_connection_t *connection, cavity_data_t *data){
    int *sock = &(connection->socket);
    request_t request;
    int retval;
    while(1){
        request = -1;
        retval = read(*sock, &request, sizeof(request_t));
        if (retval == 0){
            printf("connection closed by client\n");
            break;
        }
        handle_requests(connection, request, data);
        usleep(1000);
    }
}

void *do_socket_communication(void *args){
    cavity_data_t *data = (cavity_data_t *)args;
    socket_connection_t connection;
    connection.port = PORT;
    setup_socket_server(&connection);

    while (1){
        wait_for_connection(&connection);
        polling(&connection, data);
    }
    close(connection.socket);
}

void copy_lock_settings(lock_t *dst, lock_t *src){
    dst->P = src->P;
    dst->I = src->I;
    dst->D = src->D;
    dst->setpoint = src->lockpoint;
    dst->min = src->min;
    dst->max = src->max;
    dst->maxchange = src->maxchange;
    dst->maxstd = src->maxstd;
}

int copy_settings_network_to_local(cavity_data_t *network, cavity_data_t *local){
    local->changed_config = network->changed_config;
    local->running = network->running;
    local->run_master = network->run_master;
    local->run_slave = network->run_slave;
    if (network->changed_awg){
        local->frequency = network->frequency;
        local->amplitude = network->amplitude;
        local->offset = network->offset;
        local->changed_awg = network->changed_awg;
    }
    if (network->changed_lock_settings){
        local->changed_lock_settings = network->changed_lock_settings;
        #ifdef MASTER
            copy_lock_settings(&(local->master), &(network->master));
            if (network->changed_lock_masterprocessvalue){
                local->changed_lock_masterprocessvalue = 1;
                (local->master).processvalue = (network->master).processvalue;
            }
        #endif
        #ifdef SLAVE_CH
            copy_lock_settings(&(local->slave), &(network->slave));
            if (network->changed_lock_slaveprocessvalue){
                local->changed_lock_slaveprocessvalue = 1;
                (local->slave).processvalue = (network->slave).processvalue;
            }
        #endif
    }
    return 0;
}

int copy_local_to_network(cavity_data_t *local, cavity_data_t *network){
    network->running = local->running;
    network->changed_config = local->changed_config;
    network->changed_awg = local->changed_awg;

    #ifdef MASTER
        network->frequency = local->frequency;
        network->amplitude = local->amplitude;
        network->offset = local->offset;
        memcpy(&(network->master), &(local->master), sizeof(lock_t));
    #endif

    #ifdef SLAVE_CH
        memcpy(network->ch2, local->ch2, BUFF_SIZE*sizeof(BUFFER_TYPE));
        memcpy(network->filtered2, local->filtered2, BUFF_SIZE*sizeof(float));
        memcpy(&(network->peaks_ch2), &(local->peaks_ch2), sizeof(peaks_t));
        memcpy(&(network->slave), &(local->slave), sizeof(lock_t));
    #endif

    memcpy(network->ch1, local->ch1, BUFF_SIZE*sizeof(BUFFER_TYPE));
    memcpy(network->filtered1, local->filtered1, BUFF_SIZE*sizeof(float));
    memcpy(&(network->peaks_ch1), &(local->peaks_ch1), sizeof(peaks_t));
    return 0;
}