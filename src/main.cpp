
#include <stdio.h>
#include <thread>
#include <chrono>
#include <iostream>
#include <random>
#include <SoapySDR/Types.hpp>
#include <dsp/types.h>
#include <dsp/stream.h>
#include <dsp/block.h>
#include <dsp/blocks/routing.h>
#include <dsp/blocks/math.h>
#include <dsp/blocks/am_demod.h>
#include <dsp/blocks/fm_demod.h>
#include <dsp/blocks/firfilter.h>
#include <dsp/blocks/soapysdr_source.h>

//Comments in the code are what the programmer said when debugging

// Note: will have to use volk_32fc_s32f_power_spectrum_32f(); for the FFT dB calculation
// use volk_32fc_s32fc_x2_rotator_32fc for the sine source
// use volk_32f_s32f_32f_fm_detect_32f for FM demod (that's bad)
// Fix argument order in volk add and multiply
// add set inputs for adder and multiplier
int main() {
    printf("Hello World!\n");

    const int ROUNDS = 6;
    int iterations = 200; //Iterations per one round
    const float rf_samplerate = 2.8e6; //Samplerate of RF blocks
    
    dsp::SoapysdrSource ioin; //Init SoapySDR source block
    ioin.selectDevice(1); //Select second device by default(0 is audio input on my system)
    ioin.selectChannel(0); //Select first channel
    ioin.setSampleRate(rf_samplerate); //Set samplerate
    ioin.setAutoGain(true); //Set automatic gain
    ioin.setFrequencyCorrection(-2); //Set PPM freq correction
    ioin.setFrequency(433e6); //Tune to frequency
    ioin.initStream(); //Prepare stream for reading data
    
    dsp::FIRFilter filter(&ioin.out, rf_samplerate, 100000, 100000); //Init FIR filter object
    
    ioin.start(); //Start SoapySDR source reading thread
    filter.start(); //Start filter thread

    float speed = 0.0f;
    float etime = 0;

    for (int r = 0; r < ROUNDS; r++) {
        float samples_count = 0; //Count samples processed
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; i++) {
            samples_count += filter.out.read();
            filter.out.flush();
        } 
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start); 
        
        int rndnum = r+1;
        float timeElapsed = duration.count();
        float cycle_speed = (samples_count / timeElapsed);
        float avgspeed = speed/r;
        etime += timeElapsed;
        speed += cycle_speed;
        
        printf("Round %d, Cycle Speed: %f MS/s, Avg Speed: %f MS/s (Samples processed: %d, Round time: %d uS) \n", rndnum, cycle_speed, avgspeed, (int)samples_count, (int)timeElapsed);
    }

    printf("Average Speed: %f MS/s, Total time elapsed: %fs \n", (speed/(float)ROUNDS), (etime/1000000.0f));
    
    printf("Returning!\n");

    return 0;
}
