#ifndef CONFIG_H
#define CONFIG_H

#include <pthread.h>

// uncomment to enable networking
#define NETWORKING

// uncomment to enable PID
#define PID

// uncomment WAIT_ACQ to wait until write_pointer is done, use CONT_ACQ
// to read while writing to the ADC buffer
// CONT_ACQ is faster, but the trigger is sometimes 1 index early, so
// the buffer might be shifted w.r.t WAIT_ACQ by 1

// #define WAIT_ACQ
#define CONT_ACQ

// uncomment master or slave to set as master or slave
// master red pitaya controls the ramp and cavity offset plus a single other laser
// SLAVE controls a single other laser
#define MASTER
// #define SLAVE

// which input channels to use, uncomment for use
#define MASTER_CH RP_CH_1
#define SLAVE_CH RP_CH_2

// buffer size for ADC and DAC, maximum is 16384
#define BUFF_SIZE 16384
// type of acquisition buffer
#define BUFFER_TYPE int16_t

// socket communication port
#define PORT 8080

// maximum peaks for fixed array creation
#define NPEAKS_MAX 200

// maximum number of savgol coefficients, to be able to change the filtering on 
// the fly
#define MAX_COEFFICIENTS 100
// savgol parameters, window of 11, derivative, 1st order poly interpolation
#define WINDOW 11
#define COEFFICIENTS { -4.54545455e-02, -3.63636364e-02, -2.72727273e-02, -1.81818182e-02, -9.09090909e-03,  8.87951244e-18,  9.09090909e-03,  1.81818182e-02, 2.72727273e-02,  3.63636364e-02,  4.54545455e-02}

// threading mutex lock
extern pthread_mutex_t mutex;

// ramp config values
#define AMPLITUDE 0.5
// frequency should be roughly equivalent to the acquisition time, maybe slightly
// slower to have the fast ramp back to 0 occur while not acquiring, to be safe
// check on a scope if there is some deadtime between scans
// #define FREQUENCY 1/8.333e-3
#define FREQUENCY 1/9.3e-3
#define OFFSET 0.

// peak finder config values
#define WIDTH_MASTER 200
#define MIN_HEIGHT_MASTER 1000
#define RELATIVE_HEIGHT_MASTER 0.5
#define NFIT_MASTER 100
#define WIDTH_SLAVE 200
#define MIN_HEIGHT_SLAVE 1000
#define RELATIVE_HEIGHT_SLAVE 0.5
#define NFIT_SLAVE 100
// acquire config
// the total FPGA ADC buffer is 16384 samples
// a trigger delay of 0 means the trigger is in the center of the samples
// internally rp_AcqSetTriggerDelay(delay) adds 8192 to delay
// delay + 8129: number of decimated data after trigger written into memory.
#define TRIGGER_PIN 7
#define TRIGGER_DELAY +8192
#define DECIMATION RP_DEC_64

// lock config parameters
// at 80 Hz this would be equivalent to a 2s buffer
#define STD_ARRAY_SIZE 160
#define MASTER_P 0
#define MASTER_I 0
#define MASTER_D 0
#define MASTER_SETPOINT 0
#define MASTER_MIN 0
#define MASTER_MAX 1
#define MASTER_MAXCHANGE 0.001
#define MASTER_MAXSTD 0.001
#define SLAVE_P 0
#define SLAVE_I 0
#define SLAVE_D 0
#define SLAVE_SETPOINT 0
#define SLAVE_MIN 0
#define SLAVE_MAX 1
#define SLAVE_MAXCHANGE 0.001
#define SLAVE_MAXSTD 0.001

#endif