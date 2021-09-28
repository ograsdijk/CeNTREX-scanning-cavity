#include "pid.h"

int pid(float *lockpoint, float setpoint, float *error, float *integral, 
        float *derivative, float *processvalue, float P, float I, float D){
    float new_error = setpoint - *lockpoint;
    *derivative = new_error - *error;
    *error = new_error;
    *integral = *integral + *error;
    *processvalue = P*(*error) + I*(*integral) + D*(*derivative);
    return 0;
}

float closest_value_in_array(float value, float array[], int array_size){
    float closest = array[0];
    float distance = (value - array[0]);
    float squared_distance_smallest = distance*distance;
    int i;
    for (i = 1; i < array_size; i++){
        distance = (value - array[i]);
        if (squared_distance_smallest > distance*distance){
            closest = array[i];
            squared_distance_smallest = distance*distance;
        }
    }
    return closest;
}

int master_processvalue(cavity_data_t *cavity_data){
    // code keeps the first master peak at the same location
    peaks_t *master_peaks = &(cavity_data->peaks_ch1);
    lock_t *master_lock = &(cavity_data->master);
    float master_start = master_peaks->locs_f[0];
    master_lock->lockpoint = master_start;

    pid(&(master_lock->lockpoint), master_lock->setpoint, &(master_lock->error),
        &(master_lock->integral), &(master_lock->derivative), &(master_lock->processvalue),
        master_lock->P, master_lock->I, master_lock->D);
    return 0;
}

int slave_processvalue(cavity_data_t *cavity_data){
    // code assumes there are two peaks in the master scan
    // master lock
    peaks_t *master_peaks = &(cavity_data->peaks_ch1);
    float master_start = master_peaks->locs_f[0];
    float master_diff = master_peaks->locs_f[1] - master_peaks->locs_f[0];

    peaks_t *slave_peaks = &(cavity_data->peaks_ch2);
    lock_t *slave_lock = &(cavity_data->slave);
    float slave_peak = closest_value_in_array(
                            slave_lock->setpoint, slave_peaks->locs_f, 
                            slave_peaks->npeaks);

    slave_lock->lockpoint = (slave_peak - master_start)/master_diff;

    pid(&(slave_lock->lockpoint), slave_lock->setpoint, &(slave_lock->error),
        &(slave_lock->integral), &(slave_lock->derivative), &(slave_lock->processvalue),
        slave_lock->P, slave_lock->I, slave_lock->D);
    return 0;
}

int check_maxchange(lock_t *lock, float old_processvalue){
    float diff = lock->processvalue - old_processvalue;
    float maxchange = lock->maxchange;
    if (diff > maxchange){
        lock->processvalue = old_processvalue + maxchange;
    }
    else if (diff < maxchange){
        lock->processvalue = old_processvalue - maxchange;
    }
    return 0;
}

int check_minmax(lock_t *lock){
    if (lock->processvalue > lock->max){
        lock->processvalue = lock->max;
        lock->locked = 0;
    }
    else if (lock->processvalue < lock->min){
        lock->processvalue = lock->min;
        lock->locked = 0;
    }
    return 0;
}

void write_circular_buffer(circular_buffer_t *circular_buffer, float std){
    uint32_t *write_pointer = &(circular_buffer->write_pointer);
    uint32_t *write_count = &(circular_buffer->write_count);
    uint32_t size = circular_buffer->size;
    if (*write_count == 0){
        *write_pointer = 0;
        (circular_buffer->buffer)[*write_pointer] = std;
    }
    else{
        *write_pointer += 1;
        if (*write_pointer == size/2 - 1){
            *write_pointer = size/2;
            memcpy(circular_buffer->buffer, &((circular_buffer->buffer)[size/2]), size);
            (circular_buffer->buffer)[*write_pointer] = std;
        }
        else{
            *write_pointer += 1;
            (circular_buffer->buffer)[*write_pointer] = std;
        }
    }
    if (circular_buffer->write_count < circular_buffer->size){
        circular_buffer->write_count += 1;
    }    
}

void read_circular_buffer(float array[], int32_t *size, circular_buffer_t *circular_buffer){
    uint32_t write_pointer = circular_buffer->write_pointer;
    uint32_t write_count = circular_buffer->write_count;
    if (*size >= write_count){
        *size = write_count;
    }
    memcpy(array, &((circular_buffer->buffer)[write_pointer]), *size);
}

float calculate_std(float array[], uint32_t size){
    float sum = 0;
    float sumsqr = 0;
    uint32_t i;
    for (i=0; i<size; i++){
        sum += array[i];
        sumsqr = (array[i]*array[i]);
    }
    float var = (sumsqr - (sum*sum)/size) / (size - 1);
    return sqrt(var);
}

int do_pid(cavity_data_t *cavity_data, circular_error_buffers_t *buffers){
    float error_array[STD_ARRAY_SIZE];
    int size = STD_ARRAY_SIZE;
    lock_t *lock;
    float old_processvalue;
    #ifdef MASTER
        lock = &(cavity_data->master);
        if (cavity_data->run_master){    
            if ((cavity_data->peaks_ch1).npeaks == 2){
                old_processvalue = lock->processvalue;
                master_processvalue(cavity_data);
                check_maxchange(lock, old_processvalue);
                check_minmax(lock);
                write_circular_buffer(&(buffers->master), lock->error);
                read_circular_buffer(error_array, &size, &(buffers->master));
                lock->std = calculate_std(error_array, size);
                if (lock->std <= lock->maxstd){
                    lock->locked = 1;
                }
                else{
                    lock->locked = 0;
                }
            }
            else{
                lock->locked = 0;
            }
        }
        else{
            lock->locked = 0;
        }
    #endif
    #ifdef SLAVE_CH
        lock = &(cavity_data->slave);
        if (cavity_data->run_slave){
            if ((cavity_data->master).locked){
                old_processvalue = lock->processvalue;
                slave_processvalue(cavity_data);
                check_maxchange(lock, old_processvalue);
                check_minmax(lock);
                write_circular_buffer(&(buffers->slave), lock->error);
                read_circular_buffer(error_array, &size, &(buffers->slave));
                lock->std = calculate_std(error_array, size);
                if (lock->std <= lock->maxstd){
                    lock->locked = 1;
                }
                else{
                    lock->locked = 0;
                }
            }
            else{
                lock->locked = 0;
            }
        }
        else{
            lock->locked = 0;
        }
    #endif
    return 0;
}