#include "peak_find.h"

int find_peaks_naive(BUFFER_TYPE data[], int nsamples, peak_config_ch_t peakconfig,
                    peaks_t *peaks, int max_peaks){
    char peak = 0;
    int start = 0;
    int stop = 0;
    int i;
    for (i = 0; i < nsamples; i++){
        if (data[i] >= peakconfig.min_height){
            if (peak == 0){
                peak = 1;
                start = i;
            }
            if (peak == 1){
                stop = i;
            }
        }
        else{
            if (peak == 1){
                if (stop - start >= peakconfig.width){
                    peaks->locs[peaks->npeaks] = (start + stop)/2;
                    peaks->width_minheight[peaks->npeaks] = (stop - start);
                    peaks->npeaks = peaks->npeaks +1;
                    if (peaks->npeaks == max_peaks){
                        return -1;
                    }
                    peak = 0;
                }
                else {
                    peak = 0;
                    stop = 0;
                }
            }
        }
    }
    return 0;
}

inline static REAL sqr(REAL x) {
    return x*x;
}


int linreg(int n, const REAL x[], const REAL y[], REAL* m, REAL* b, REAL* r){
    REAL   sumx = 0.0;                      /* sum of x     */
    REAL   sumx2 = 0.0;                     /* sum of x**2  */
    REAL   sumxy = 0.0;                     /* sum of x * y */
    REAL   sumy = 0.0;                      /* sum of y     */
    REAL   sumy2 = 0.0;                     /* sum of y**2  */

    for (int i=0;i<n;i++){ 
        sumx  += x[i];       
        sumx2 += sqr(x[i]);  
        sumxy += x[i] * y[i];
        sumy  += y[i];      
        sumy2 += sqr(y[i]); 
    } 

    REAL denom = (n * sumx2 - sqr(sumx));
    if (denom == 0) {
        // singular matrix. can't solve the problem.
        *m = 0;
        *b = 0;
        if (r) *r = 0;
            return 1;
    }

    *m = (n * sumxy  -  sumx * sumy) / denom;
    *b = (sumy * sumx2  -  sumx * sumxy) / denom;
    if (r!=NULL) {
        *r = (sumxy - sumx * sumy / n) /    /* compute correlation coeff */
                sqrt((sumx2 - sqr(sumx)/n) *
                (sumy2 - sqr(sumy)/n));
    }

    return 0; 
}

void savgol_filter(int start, int stop, int window, float coefficients[], BUFFER_TYPE data[], float filtered[]){
    int half_window = window/2;
    int i,j;
    for (i = start; i < stop; i = i+1){
        for (j = 0; j < window; j = j+1){
            filtered[i] += coefficients[j] * (float)data[i-half_window+j];
        }
    }
}

float maximum(float array[], int arrsize){
    float max = array[0];
    int i;
    for (i = 0; i<arrsize; i++){
        if (array[i] > max){
            max = array[i];
        }
    }
    return max;
}

void fit_zero_der(BUFFER_TYPE data[], int start, int stop, savgol_config_t savgol, 
                    float filtered[], float* zero_crossing){
    int window = savgol.window;
    savgol_filter(start - window, stop + window, window, savgol.coefficients, data, filtered);

    const int nfit = stop - start;
    float *xdata = (float *)malloc(nfit * sizeof(float));
    int j;
    for (j = 0; j < nfit; j++){
        xdata[j] = j;
    }
    REAL m,b,r;
    linreg(nfit,xdata,filtered+start,&m,&b,&r);
    *zero_crossing = start + -b/m;
}

void find_peaks_deriv(BUFFER_TYPE data[], int nfit, peaks_t *peaks, savgol_config_t savgol, 
                    float filtered[]){
    // comparator approach with simple threshold and minimum width
    int p;
    for (p = 0; p < peaks->npeaks; p++){
        int start = peaks->locs[p] - nfit/2;
        int stop = peaks->locs[p] + nfit/2;
        fit_zero_der(data, start, stop, savgol, filtered, &(peaks->locs_f)[p]);
    }
}

int peak_find(cavity_data_t *cavity_data, peak_config_t peak_config, 
                savgol_config_t savgol){
    #ifdef MASTER_CH
        find_peaks_naive(cavity_data->ch1, BUFF_SIZE, peak_config.ch1, 
                        &(cavity_data->peaks_ch1), NPEAKS_MAX);
        if ((cavity_data->peaks_ch1).npeaks){
            find_peaks_deriv(cavity_data->ch1, (peak_config.ch1).nfit, &(cavity_data->peaks_ch1), savgol, 
                            cavity_data->filtered1);
        }
    #endif
    #ifdef SLAVE_CH
        find_peaks_naive(cavity_data->ch2, BUFF_SIZE, peak_config.ch2, 
                        &(cavity_data->peaks_ch2), NPEAKS_MAX);
        if ((cavity_data->peaks_ch2).npeaks){
            find_peaks_deriv(cavity_data->ch2, (peak_config.ch2).nfit, &(cavity_data->peaks_ch2), savgol, 
                            cavity_data->filtered2);
        }
    #endif
    return 0;
}