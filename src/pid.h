#ifndef PID_H
#define PID_H

#include <math.h>
#include <string.h>
#include "data_structs.h"
#include "config.h"

int pid(float *lockpoint, float setpoint, float *error, float *integral, 
        float *derivative, float *control, float P, float I, float D);
/**
 * @brief find array element closest to value
 * 
 * @param value value to find closest array element to
 * @param array array
 * @param array_size array size
 * @return float closest value in array
 */
float closest_value_in_array(float value, float array[], int array_size);
int master_processvalue(cavity_data_t *cavity_data);
int slave_processvalue(cavity_data_t *cavity_data);
/**
 * @brief check if PID control doesn't exceed the maximum allowed change
 * 
 * @param lock lock_t struct containing the lock information
 * @param old_processvalue old process value
 * @return int 
 */
int check_maxchange(lock_t *lock, float old_processvalue);
int check_minmax(lock_t *lock);
void write_circular_buffer(circular_buffer_t *circular_buffer, float std);
void read_circular_buffer(float array[], int32_t *size, circular_buffer_t *circular_buffer);
float calculate_std(float array[], uint32_t size);
int do_pid(cavity_data_t *cavity_data, circular_error_buffers_t *buffers);

#endif