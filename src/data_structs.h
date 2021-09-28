#ifndef DATA_STRUCTS_H
#define DATA_STRUCTS_H

#include "config.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

typedef struct{
    int window;
    float coefficients[MAX_COEFFICIENTS];
} savgol_config_t;

typedef struct{
    int width;
    float min_height;
    float relative_height;
    int nfit;
} peak_config_ch_t;

typedef struct{
    peak_config_ch_t ch1;
    peak_config_ch_t ch2;
} peak_config_t;

typedef struct{
    int32_t npeaks;
    BUFFER_TYPE locs[NPEAKS_MAX];
    float locs_f[NPEAKS_MAX];
    int width_minheight[NPEAKS_MAX];
} peaks_t;

typedef struct{
    float buffer[2*STD_ARRAY_SIZE];
    uint32_t write_pointer;
    uint32_t write_count;
    uint32_t size;
} circular_buffer_t;

typedef struct{
    circular_buffer_t master;
    circular_buffer_t slave;
} circular_error_buffers_t;

typedef struct{
    char locked;
    float P;
    float I;
    float D;
    float setpoint;
    float lockpoint;
    float error;
    float integral;
    float derivative;
    float processvalue;
    float std;
    float min;
    float max;
    float maxchange;
    float maxstd;
} lock_t;

typedef struct{
    char running;
    char run_master;
    char run_slave;
    char changed_config;
    char changed_awg;
    char changed_lock_settings;
    char changed_lock_masterprocessvalue;
    char changed_lock_slaveprocessvalue;
    BUFFER_TYPE ch1[BUFF_SIZE];
    BUFFER_TYPE ch2[BUFF_SIZE];
    float filtered1[BUFF_SIZE];
    float filtered2[BUFF_SIZE];
    float frequency;
    float amplitude;
    float offset;
    peaks_t peaks_ch1;
    peaks_t peaks_ch2;
    lock_t master;
    lock_t slave;
} cavity_data_t;

#endif