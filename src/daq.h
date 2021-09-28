#ifndef DAQ_H
#define DAQ_H

#include "rp.h"
#include "data_structs.h"
#include <string.h>
#include "ramp.h"
#include "config.h"

void setup_acq();
void setup_dio_trigger(int pin_trigger);
int setup_ramp(rp_channel_t ch, float frequency, float amplitude, float offset);
int setup_slave_control(rp_channel_t ch, float amplitude, float offset);
int update_ramp(rp_channel_t ch, float frequency, float amplitude, float offset);
int acquire(cavity_data_t *cavity_data);
#endif