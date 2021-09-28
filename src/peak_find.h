#ifndef PEAK_FIND_H
#define PEAK_FIND_H

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include "data_structs.h"
#include <math.h>
#include "config.h"

// define for linreg function
#define REAL float

int find_peaks_naive(BUFFER_TYPE data[], int nsamples, peak_config_ch_t peakconfig,
                    peaks_t *peaks, int max_peaks);
int linreg(int n, const REAL x[], const REAL y[], REAL* m, REAL* b, REAL* r);
void savgol_filter(int start, int stop, int window, float coefficients[], BUFFER_TYPE data[], float filtered[]);
float maximum(float array[], int arrsize);
void fit_zero_der(BUFFER_TYPE data[], int start, int stop, savgol_config_t savgol, 
                    float filtered[], float* zero_crossing);
void find_peaks_deriv(BUFFER_TYPE data[], int nfit, peaks_t *peaks, savgol_config_t savgol, 
                    float filtered[]);
int peak_find(cavity_data_t *cavity_data, peak_config_t peak_config, savgol_config_t savgol);

#endif