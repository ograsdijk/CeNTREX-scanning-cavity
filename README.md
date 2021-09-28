# CeNTREX-scanning-cavity
`Note that this software has not been run yet with PID control enabled (no cavity/lasers at home) so bugs are bound to be present.`


Scanning transfer cavity lock implemented on the Red Pitaya FPGA.

The general principle is as follows:
* `OUT 1` drives a piezo to ramp the cavity length
* `IN 1` & `2` capture the photodiode signals of the `MASTER` and `SLAVE` lasers
* a peak finding algorithm determines the peak locations of the `MASTER` and `SLAVE`
* the `MASTER` (`OUT 1` ramp offset) PID loop keeps the first peak of the `MASTER` laser at the setpoint
* the `SLAVE` (`OUT 1`, connected to the `SLAVE` laser piezo) PID loop keeps the relative location w.r.t. the two `MASTER` peaks at the setpoint 

The ramp and acquisition are triggered by an external trigger source, which is set to a `DIOX_P` pin on the Red Pitaya, which can be set in `config.h` by `#define TRIGGER_PIN X` where `X` indicates `DIO` pin number. The external input trigger source channel is not configurable, and is always set to `RP_DIO0_P` on the Red Pitaya running the standard OS and FPGA bitfile.

Each transfer lock cavity needs at least one Red Pitaya to control the cavity scan and can control the cavity (`MASTER`) and an aditional `SLAVE` laser.
Each subsequent slave laser requires an additional Red Pitaya, but these only have a PID loop controlling the slave laser. The external trigger input source has to be connected to the `MASTER` Red Pitaya unit, and it requires to be connected to the `MASTER` laser photodiode to deterine the `MASTER` peak locations.

## Installation
The `Makefile` in the `src` directory compiles the scanning cavity code on the Red Pitaya.

## Usage
The locking program runs stand-alone on the Red Pitaya, but can be monitored and controlled using a socket interface. By default the transfer lock is not running upon startup.  
In `communications.h` the communications options are described in the `enum` `requests_t`. An example of a client is shown below:
```C
#include "communications.h"
#include "data_structs.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int setup_socket_client(socket_connection_t *connection){
    if ((connection->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }
    
    (connection->address).sin_family = AF_INET;
    (connection->address).sin_port = htons(PORT);
    
    // Convert IPv4 and IPv6 addresses from text to binary form
    // for this function it only connects to the localhost
    if(inet_pton(AF_INET, "127.0.0.1", &((connection->address).sin_addr))<=0) 
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
    
    if (connect((connection->socket), (struct sockaddr *)&(connection->address), sizeof(connection->address)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }
    return 0;
}

int main(){
    BUFFER_TYPE ch1[BUFF_SIZE];
    cavity_data_t data;
    int32_t npeaks;
    int32_t array_size;
    float frequency;
    char running;
    int32_t nbytes;
    

    socket_connection_t connection;
    connection.port = PORT;
    
    setup_socket_client(&connection);
    int sock = connection.socket;

    // send the request to get the number of peaks for CH1 (MASTER)
    write_socket_int32(sock, READ_NPEAKS1);
    // read the number of peaks
    read_socket_int32(sock, &npeaks);
    printf("npeaks = %d\n",npeaks);

    // send the request to get the cavity scan frequency
    write_socket_int32(sock, READ_FREQUENCY);
    // read the cavity scan frequency
    read_socket_float(sock, &frequency);
    printf("frequency = %f\n", frequency);

    // send the request to get the CH1 data 
    write_socket_int32(sock, READ_CH1);
    // read the CH1 array size
    read_socket_int32(sock, &array_size);
    // get the CH1 data
    read_socket_bytes(sock, ch1, (array_size)*sizeof(BUFFER_TYPE));

    // send the request to read all data in one go; this is faster than reading
    // each attribute separately since all bytes are sent in a continuous stream
    write_socket_int32(sock, READ_ALL);
    // get all data
    read_socket_bytes(sock, &data, sizeof(cavity_data_t));
    printf("scanning cavity frequency = %f\n", data.frequency);
    printf("scanning cavity running = %d\n", data.running);
    printf("ch1 value at point 4000 = %d\n", data.ch1[4000]);
    printf("master npeaks = %d\n", (data.peaks_ch1).npeaks);

    return 0;
}
```
A few things cannot be controlled through the socket interface, and are currently determined at compile time from `config.h`, see the `Configuration Options` section for more details: 
* `MASTER` / `SLAVE` mode (`#define MASTER` or `#define SLAVE`)
* input/output channel assignment (`#define MASTER_CH RP_CH_1` and `#define SLAVE_CH RP_CH2`)
* continous vs wait acquition mode (`#define CONT_ACQ` or `#define WAIT_ACQ`)
* ADC decimation, i.e. sample rate (`#define DECIMATION RP_DEC_64`)
* ADC samples to read (`#define BUFF_SIZE 16384`)
* external trigger pin (`#define TRIGGER_PIN 7`)
* trigger delay (`#define TRIGGER_DELAY +8192`)
* TCP communication port (`#define PORT 8000`)
* any parameter that determines an array size, e.g. number of peaks, the std circular buffer etc. 

The data structures are defined in `data_structs.h`. This socket interface is ambiguous to which program sends or reads data to/from the Red Pitaya, and a Python socket interface can be written to control the transfer lock. The main structure is `cavity_data_t`:
```C
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
```
* the `char` values are various configuration flags, i.e. whether the ramp is running, the `MASTER` and/or `SLAVE` lasers are controlled by the PID loop, whether any config values have been changed through the socket communication.  
* `BUFFER_TYPE chX[BUFF_SIZE` contains the raw ADC data.
* `float filteredX[BUFF_SIZE]` contains the data after the Savitzky-Golay filtering. Keep in mind only data around peaks is put through the filter so the array contains zeros where no peaks are found in the initial theshold method (see Peak Finder).
* `frequency`, `amplitude` and `offset` are the ramp parameters.
* `peaks_t peaks_chX` is a struct containing the peak information
* `lock_t master` / `lock_t slave` contain the lock information for the `MASTER` / `SLAVE` lasers. 


## Peak Finder
The peak finder works by finding all subsequent points above the threshold set by `MIN_HEIGHT_MASTER` / `MIN_HEIGHT_SLAVE` and checking that the width is larger than `WIDTH_MASTER` / `WIDTH_SLAVE`. The center the two crossings of the `MIN_HEIGHT` threshold is uses as the inital guess for the peak center. A [Savitzky-Golay filter](https://en.wikipedia.org/wiki/Savitzky%E2%80%93Golay_filter) is then used to smooth the area around the peak and take the derivative. A linear least-squares fit to `y = ax+b` is then used to determine the zero crossing. 

## Configuration Options
`config.h` contains the configuration options for the transfer lock cavity at compile time, which are listed below:  
### `MASTER`/`SLAVE` configuration
* `#define MASTER` / `#define SLAVE` only ever have one of these lines uncommented, determines whether the Red Pitaya.  
### Trigger settings:  
* `#define TRIGGER_PIN 7` sets the trigger output pin
* `#define TRIGGER_DELAY +8192` sets the trigger delay. Default 0 captures 8192 samples before and after the trigger, +8192 delay captures 16384 samples after the trigger.  
### Networking settings:
* `#define NETWORKING` comment this line to disable network control of the transfer cavity lock.
* `#define PORT 8000` sets the TCP port for socket communication and control of the transver cavity.  
### Acquisition settings:
* `#define WAIT_ACQ` / `#define CONT_ACQ` only ever have one of these lines uncommented, determines whether the data acquisition runs synchronous to the piezo ramp or waits until the ramp has finished. `CONT_ACQ` is much faster.controls the cavity or only a `SLAVE` laser.
* `#define MASTER_CH RP_CH_1` set the `MASTER` laser input channel
* `#define SLAVE_CH RP_CH_2` set the `SLAVE` laser input channel
* `#define BUFF_SIZE 16384` set the # of ADC samples to acquire, 16384 is the maximum allowed
* `#define BUFFER_TYPE int16_t` set the data type of ADC acquisition. Functions reading out the voltage are slower than only reading out the input voltage.
* `#define DECIMATION RP_DEC_64` sets the ADC decimation setting, `RP_DEC_64` is equivalent to 8.333 ms.
### Ramp settings:
* `#define AMPLITUDE 0.5` sets the ramp amplitude in V, maximum is 1.
* `#define FREQUENCY 1/9.3e-3` sets the ramp frequency in Hz.
* `#define OFFSET` sets the ramp offset in V, can range from -1 to 1.
### Peak finder settings
* `#define NPEAKS_MAX` sets the maximum expected # of peaks, just used to fix the peak location array sizes at compile time.
* `#define MAX_COEFFICIENTS 100` sets the maximum expected savitzky-golay filter parameters at compile time.
* `#define WINDOW 11` sets the window size to use for the savitzky-golay filter.
* `#define COEFFICIENTS { ... }` set the savitzky-golay filter parameters, the default is the 1st derivative and linear interpolation.

* `#define WIDTH_MASTER 200` sets the minimum width for a peak in for the `MASTER` laser.
* `#define MIN_HEIGHT_MASTER 1000` sets the minimum height for a peak for the `MASTER` laser, in whatever units the readout function uses.
* `#define NFIT_MASTER` sets the number of points of the derivative around the suspected peak location to perform a linear fit to get a closer approximation of the peak center.  
Each of the peak config settings also exist for the `SLAVE` laser, swapping `MASTER` for `SLAVE`
### PID settings
* `#define PID` comment this line to disable the PID loops, included for testing purposes only.
* `#define MASTER_P X` sets the P coefficient (`X`) for the `MASTER` laser.
* `#define MASTER_I X` sets the I coefficient (`X`) for the `MASTER` laser.
* `#define MASTER_D X` sets the D coefficient (`X`) for the `MASTER` laser.
* `#define MASTER_SETPOINT X` sets the setpoint (`X`) for the `MASTER` laser, where `X` is expressed in units of the location ranging from 0 to `BUFF_SIZE`
* `#define MASTER_MIN X` sets the minimum allowed output voltage.
* `#define MASTER_MAX X` sets the maximum allowed output voltage.
* `#define MASTER_MAXCHANGE X` sets the maximum allowed change that the PID loop is allowed to output.
* `#define MASTER_MAXSTD X` sets the maximum allowed σ for which the laser is considered locked.  
Each of these settings is also present for the `SLAVE` laser, by swapping out `SLAVE` for `MASTER`
* `#define STD_ARRAY_SIZE` sets the array size for standard deviations to keep in memory. At 80 Hz this would be equivalent to 2s.