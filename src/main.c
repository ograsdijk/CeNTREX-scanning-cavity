#include "communication.h"
#include "peak_find.h"
#include "daq.h"
#include "pid.h"
#include "data_structs.h"
#include <stdio.h>
#include <pthread.h>
#include "rp.h"
#include "config.h"


// run with LD_LIBRARY_PATH=/opt/redpitaya/lib ./main.out

int update_settings(cavity_data_t *cavity_data){
    #ifdef MASTER
        if (cavity_data->changed_awg){
            update_ramp(MASTER_CH, cavity_data->frequency, cavity_data->amplitude, 
                        cavity_data->offset);
            cavity_data->changed_awg = 0;
        }
        if (cavity_data->changed_lock_masterprocessvalue){
            rp_GenOffset(MASTER_CH, (cavity_data->master).processvalue);
            cavity_data->changed_lock_masterprocessvalue = 0;
        }
    #endif
    #ifdef SLAVE_CH
        if (cavity_data->changed_lock_slaveprocessvalue){
            rp_GenOffset(SLAVE_CH, (cavity_data->slave).processvalue);
            rp_GenOutEnable(SLAVE_CH);
            cavity_data->changed_lock_slaveprocessvalue = 0;
        }
    #endif
    cavity_data->changed_config = 0;
    cavity_data->changed_lock_settings = 0;
    return 0;
}

int main(){
    printf("starting cavity lock\n");
    
    #ifdef MASTER
        printf("running in MASTER mode, controlling the cavity ramp\n");
    #endif

    // reload the fpga bit file
    system("cat /opt/redpitaya/fpga/fpga_0.94.bit > /dev/xdevcfg");

    // initialize cavity data
    cavity_data_t cavity_data;
    #ifdef NETWORKING
        cavity_data_t network_cavity_data;
    #endif
    #ifdef PID
        // instantiate circular error buffers for PID
        circular_error_buffers_t error_buffers;
        #ifdef MASTER
            (error_buffers.master).size = STD_ARRAY_SIZE;
        #endif
        (error_buffers.slave).size = STD_ARRAY_SIZE;
    #endif

    cavity_data.running = 0;
    cavity_data.run_master = 0;
    cavity_data.run_slave = 0;

    // // initialize peak config
    peak_config_t peak_config;
    peak_config.ch1.width = WIDTH_MASTER;
    peak_config.ch1.min_height = MIN_HEIGHT_MASTER;
    peak_config.ch1.relative_height = RELATIVE_HEIGHT_MASTER;
    peak_config.ch1.nfit = NFIT_MASTER;
    #ifdef SLAVE_CH
        peak_config.ch2.width = WIDTH_SLAVE;
        peak_config.ch2.min_height = MIN_HEIGHT_SLAVE;
        peak_config.ch2.relative_height = RELATIVE_HEIGHT_SLAVE;
        peak_config.ch2.nfit = NFIT_SLAVE;
    #endif

    // initialize savgol filter config
    savgol_config_t savgol = {
        .window = WINDOW, 
        .coefficients = COEFFICIENTS
    };

    // initialize lock config
    #ifdef MASTER
        lock_t *master = &(cavity_data.master);
        master->P = MASTER_P;
        master->I = MASTER_I;
        master->D = MASTER_D;
        master->setpoint = MASTER_SETPOINT;
        master->min = MASTER_MIN;
        master->max = MASTER_MAX;
        master->maxchange = MASTER_MAXCHANGE;
        master->maxstd = MASTER_MAXSTD;
    #endif
    #ifdef SLAVE_CH
        lock_t *slave = &(cavity_data.slave);
        slave->P = SLAVE_P;
        slave->I = SLAVE_I;
        slave->D = SLAVE_D;
        slave->setpoint = SLAVE_SETPOINT;
        slave->min = SLAVE_MIN;
        slave->max = SLAVE_MAX;
        slave->maxchange = SLAVE_MAXCHANGE;
        slave->maxstd = SLAVE_MAXSTD;
    #endif

    #ifdef NETWORKING
        // intialize socket communication thread
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, &do_socket_communication, (void *)&network_cavity_data);
    #endif

    // // reset the red pitaya to default settings
    rp_InitReset(1);

    /* Print error, if rp_Init() function failed */
    if(rp_Init() != RP_OK){
            fprintf(stderr, "Rp api init failed!\n");
    }
    rp_Reset();

    // reset DAC
    /*
    The ramp wasn't working but after playing around with it, and without
    changing any of the functions it's working now.
    Perhaps the order of the function calls made a difference.
    Works with either RP_WAVEFORM_RAMP_UP or RP_WAVEFORM_ARBITRARY
    */
    rp_GenReset();
    #ifdef MASTER
        cavity_data.frequency = FREQUENCY;
        cavity_data.amplitude = AMPLITUDE;
        cavity_data.offset = OFFSET;
        setup_ramp(MASTER_CH, FREQUENCY, AMPLITUDE, OFFSET);
        setup_ramp(SLAVE_CH, FREQUENCY, AMPLITUDE, OFFSET);

        // setup digital trigger output
        setup_dio_trigger(TRIGGER_PIN);
        sleep(1);

        rp_GenOutEnable(MASTER_CH);
        rp_GenOutEnable(SLAVE_CH);
    #endif
    // #ifdef SLAVE_CH
    //     setup_slave_control(SLAVE_CH, 1, slave->offset);
    // #endif

    setup_acq();
    // adc continously writes to a circular buffer of size 16384
    // when the trigger is received it writes TRIGGER_DELAY more points into the
    // buffer
    rp_AcqStart();

    cavity_data.running = 0;
    while(1){
        if (cavity_data.running){
            #ifdef MASTER
                // trigger for ramp and acquisition
                rp_DpinSetState(RP_DIO0_P+TRIGGER_PIN, RP_HIGH);
            #endif
            #ifdef NETWORKING
                // have to wait for buffer to fill, copy data and check for new settings
                // mutex lock to prevent simultanous read/write when copying data
                pthread_mutex_lock(&mutex);
                // update local settings if network settings were changed
                if (network_cavity_data.changed_config){
                    copy_settings_network_to_local(&network_cavity_data, &cavity_data);
                    update_settings(&cavity_data);
                }
                // copy local to network
                copy_local_to_network(&cavity_data, &network_cavity_data);
                // unlock network_cavity_data
                pthread_mutex_unlock(&mutex);
            #endif
            
            // acquire data from ADC
            acquire(&cavity_data);

            #ifdef MASTER
                // set trigger pin low
                rp_DpinSetState(RP_DIO0_P+TRIGGER_PIN, RP_LOW);
            #endif

            // find peak locations
            peak_find(&cavity_data, peak_config, savgol);

            #ifdef PID
                // calculate the PID values
                do_pid(&cavity_data, &error_buffers);

                // set processvalues to DAC outputs
                #ifdef MASTER
                    if (cavity_data.run_master){
                        rp_GenOffset(MASTER_CH, (cavity_data.master).processvalue);
                    }
                #endif
                #ifdef SLAVE_CH
                    if (cavity_data.run_slave){
                        rp_GenOffset(SLAVE_CH, (cavity_data.slave).processvalue);
                    }
                #endif
            #endif

            // start acquisition again
            rp_AcqStart();
            rp_AcqSetTriggerSrc(RP_TRIG_SRC_EXT_PE);
        }
        #ifdef NETWORKING
            else{
                pthread_mutex_lock(&mutex);
                if (network_cavity_data.changed_config){
                    copy_settings_network_to_local(&network_cavity_data, &cavity_data);
                    update_settings(&cavity_data);
                }
                // copy local to network
                copy_local_to_network(&cavity_data, &network_cavity_data);
                pthread_mutex_unlock(&mutex);
                usleep(10000);
            }
        #endif
    }
    rp_Release();
    return 0;
}