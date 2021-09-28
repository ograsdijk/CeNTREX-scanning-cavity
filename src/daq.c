#include "daq.h"
#include <stdio.h>

float ramp[BUFF_SIZE] = RAMP;

int setup_ramp(rp_channel_t ch, float frequency, float amplitude, float offset){
    // rp_GenWaveform(ch, RP_WAVEFORM_RAMP_UP);
    rp_GenFreq(ch, frequency);
    rp_GenAmp(ch, amplitude);
    rp_GenOffset(ch, offset);
    rp_GenWaveform(ch, RP_WAVEFORM_ARBITRARY);
    rp_GenArbWaveform(ch, ramp, BUFF_SIZE);
    rp_GenMode(ch, RP_GEN_MODE_BURST);
    rp_GenBurstCount(ch, 1);
    rp_GenBurstRepetitions(ch, 1);
    // trigger the AWG on the positive edge of the external trigger input
    // DIO0_P on extension connector E1
    rp_GenTriggerSource(ch, RP_GEN_TRIG_SRC_EXT_PE);
    return 0;
}

int setup_slave_control(rp_channel_t ch, float amplitude, float offset){
    rp_GenWaveform(ch, RP_WAVEFORM_DC);
    rp_GenAmp(ch, amplitude);
    rp_GenOffset(ch, offset);
    rp_GenTriggerSource(ch, RP_GEN_TRIG_SRC_INTERNAL);
    return 0;
}

int update_ramp(rp_channel_t ch, float frequency, float amplitude, float offset){
    rp_GenFreq(ch, frequency);
    rp_GenAmp(ch, amplitude);
    rp_GenOffset(ch, offset);
    return 0;
}

void setup_acq(){
    rp_AcqReset();
    // rp_AcqResetFpga();
    rp_AcqSetDecimation(DECIMATION);
    rp_AcqSetTriggerDelay(TRIGGER_DELAY);
    rp_AcqSetTriggerSrc(RP_TRIG_SRC_EXT_PE);
}

void setup_dio_trigger(int pin_trigger){
    rp_DpinSetDirection(RP_DIO0_P+pin_trigger, RP_OUT);
    rp_DpinSetDirection(RP_DIO0_P, RP_IN);
}

int acquire(cavity_data_t *cavity_data){
    uint32_t write_pointer;
    uint32_t write_pointer_prev = BUFF_SIZE+1;
    rp_acq_trig_state_t state = RP_TRIG_STATE_TRIGGERED;

    // zero out filtered for use in savgol
    memset(cavity_data->filtered1, 0, BUFF_SIZE*sizeof(*cavity_data->filtered1));
    #ifdef SLAVE_CH
        memset(cavity_data->filtered2, 0, BUFF_SIZE*sizeof(*cavity_data->filtered2));
    #endif

    // reset npeaks to 0
    (cavity_data->peaks_ch1).npeaks = 0;
    #ifdef SLAVE_CH
        (cavity_data->peaks_ch2).npeaks = 0;
    #endif

    // wait for trigger
    while(1){
        if(state == RP_TRIG_STATE_TRIGGERED){    
                rp_AcqGetTriggerState(&state);
                break;
        }
    }
    #ifdef WAIT_ACQ
        rp_acq_trig_src_t source = RP_TRIG_SRC_CHA_PE;
        while(1){
            rp_AcqGetTriggerSrc(&source);
            if(source == RP_TRIG_SRC_DISABLED){
                    break;
            }
        }   
        // the above two loops exit when the trigger is stopped, but since we
        // want to acquire samples after the trigger we have to check if the 
        // buffer is still written to, the usleep is present to prevent issues
        // with reading consecutively before the write_pointer is updated
        uint32_t buff_size = BUFF_SIZE;
        while(1){
            rp_AcqGetWritePointer(&write_pointer);
            if(write_pointer == write_pointer_prev){
                break;
            }
            else{
                write_pointer_prev = write_pointer;
            }
            usleep(1);
        }
        rp_AcqGetLatestDataRaw(RP_CH_1, &buff_size, cavity_data->ch1);
        #ifdef SLAVE_CH
            buff_size = BUFF_SIZE;
            rp_AcqGetLatestDataRaw(RP_CH_2, &buff_size, cavity_data->ch2);
        #endif
    #endif

    #ifdef CONT_ACQ
        rp_AcqGetWritePointerAtTrig(&write_pointer_prev);
        uint32_t vals_read = 0;
        uint32_t vals_to_read;
        do {
            rp_AcqGetWritePointer(&write_pointer);
            if (write_pointer < write_pointer_prev){
                vals_to_read = BUFF_SIZE - write_pointer_prev + write_pointer;
            }
            else{
                vals_to_read = write_pointer - write_pointer_prev;
            }
            // sometimes the scope writes one more value after the buffer
            // check if this occurs and read one less value
            // haven't figured out why yet, but this will only shift the positions
            // by one dt, which only matters for the cavity lock (and is within noise), 
            // the slave lock is calibrated to the dual peak positions
            if (vals_to_read + vals_read > BUFF_SIZE){
                vals_to_read -= 1;
            }
            #ifdef SLAVE_CH
                rp_AcqGetDataRaw(MASTER_CH, write_pointer_prev, &vals_to_read, 
                                &((cavity_data->ch1)[vals_read]));
                rp_AcqGetDataRaw(SLAVE_CH, write_pointer_prev, &vals_to_read, 
                                &((cavity_data->ch2)[vals_read])); 
            #endif
            #ifndef SLAVE_CH
                rp_AcqGetDataRaw(write_pointer, vals_to_read, 
                                (cavity_data->ch1)[vals_read]);
            #endif
            write_pointer_prev = write_pointer;
            vals_read += vals_to_read;
            if (vals_read > BUFF_SIZE){
                exit(EXIT_FAILURE);
            }
            // usleep(1);
        } while( vals_read != BUFF_SIZE);
    #endif
    return 0;
}
