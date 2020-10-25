
#include <stdio.h>
#include <dsp/stream.h>
#include <dsp/block.h>
#include <dsp/routing.h>
#include <dsp/math.h>
#include <dsp/demodulator.h>
#include <thread>
#include <chrono>
#include <iostream>

// Note: will have to use volk_32fc_s32f_power_spectrum_32f(); for the FFT dB calculation
// use volk_32fc_s32fc_x2_rotator_32fc for the sine source
// use volk_32f_s32f_32f_fm_detect_32f for FM demod (that's bad)
// Fix argument order in volk add and multiply
// add set inputs for adder and multiplier

int main() {
    printf("Hello World!\n");

    dsp::stream<dsp::complex_t> in_stream;
    dsp::FMDemod demod(&in_stream, 1000000, 100000);

    demod.start();

    while(1) {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 1000; i++) {
            in_stream.aquire();
            in_stream.write(1000000);
            
            demod.out.read();
            demod.out.flush();
        } 
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start); 

        

        double dur = duration.count();
        double sps = ((1000000.0 / dur) * 1000000000.0) / 1000000.0;
  
        std::cout << "Speed: " << sps << " MS/s" << std::endl; 
    }

    printf("Returning!\n");

    return 0;
}