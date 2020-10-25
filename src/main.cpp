
#include <stdio.h>
#include <dsp/stream.h>
#include <dsp/block.h>
#include <dsp/routing.h>
#include <dsp/math.h>
#include <dsp/demodulator.h>
#include <dsp/filter.h>
#include <thread>
#include <chrono>
#include <iostream>
#include <random>

// Note: will have to use volk_32fc_s32f_power_spectrum_32f(); for the FFT dB calculation
// use volk_32fc_s32fc_x2_rotator_32fc for the sine source
// use volk_32f_s32f_32f_fm_detect_32f for FM demod (that's bad)
// Fix argument order in volk add and multiply
// add set inputs for adder and multiplier

int main() {
    printf("Hello World!\n");

    const int ROUNDS = 1;

    dsp::stream<dsp::complex_t> in_stream;
    dsp::PolyphaseFIR<32> demod(&in_stream, 1000000, 500000, 10000);

    for (int i = 0; i < 1000000; i++) {
        in_stream.data[i].i = float(rand())/float((RAND_MAX)) - 1.0f;;
        in_stream.data[i].q = float(rand())/float((RAND_MAX)) - 1.0f;;
    }

    demod.start();

    float speed = 0.0f;

    for (int r = 0; r < ROUNDS; r++) {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 1000; i++) {
            in_stream.aquire();
            in_stream.write(1000000);
            
            demod.out.read();
            demod.out.flush();
        } 
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start); 

        speed += ((1000000.0 / duration.count()) * 1000000000.0) / 1000000.0;
        std::cout << "Round " << r + 1 << std::endl;
    }

    std::cout << "Average Speed: " << (speed/(float)ROUNDS) << " MS/s" << std::endl;

    printf("Returning!\n");

    return 0;
}