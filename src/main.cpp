
#include <stdio.h>
#include <thread>
#include <chrono>
#include <iostream>
#include <random>
#include <spdlog/spdlog.h>
#include <wav.h>

#include <dsp/convertion.h>
#include <dsp/processing.h>
#include <dsp/resampling.h>
#include <dsp/filter.h>
#include <dsp/sink.h>

#define SAMPLE_COUNT    (5 * 6000)
#define RUN_COUNT       100

int main() {
    printf("Hello World!\n");

    spdlog::info("Creating random buffer");
    float* randBuf =  new float[SAMPLE_COUNT];
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        randBuf[i] = static_cast <float> (rand()) / ( static_cast <float> (RAND_MAX/2)) - 1;
    }

    spdlog::info("Benchmarking");

    // ======= DUT =======
    dsp::stream<float> inputStream;

    dsp::filter_window::BlackmanWindow win(1000, 100, 6000);
    dsp::PolyphaseResampler<float> resamp(&inputStream, &win, 6000, 48000);
    win.setSampleRate(6000 * resamp.getInterpolation());
    resamp.updateWindow(&win);

    resamp.start();
    
    WavWriter wavWriter("output.wav", 16, 1, 48000);
    // ===================


    // Write to input
    memcpy(inputStream.writeBuf, randBuf, SAMPLE_COUNT * sizeof(float));
    inputStream.swap(SAMPLE_COUNT);

    // Read from output
    int count = resamp.out.read();
    int16_t* samples = new int16_t[count];
    for (int i = 0; i < count; i++) {
        samples[i] = resamp.out.readBuf[i] * 10000.0f;
    }
    wavWriter.writeSamples(samples, count * sizeof(int16_t));
    wavWriter.close();

    

    count = win.getTapCount();
    float* taps = new float[count];
    win.createTaps(taps, count);
    for (int i = 0; i < count; i++) {
        printf("%f\n", taps[i]);
    }








    // double totalWait = 0.0f;

    // for (int i = 0; i < RUN_COUNT; i++) {
    //     auto start = std::chrono::high_resolution_clock::now();

    //     memcpy(inputStream.writeBuf, randBuf, SAMPLE_COUNT * sizeof(float));
    //     inputStream.swap(SAMPLE_COUNT);

    //     auto stop = std::chrono::high_resolution_clock::now();

    //     auto delay = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    //     totalWait += delay.count();
    // }

    // double avgWait = totalWait / (double)RUN_COUNT;

    // spdlog::info("Average run took {0} uS ({1} MS/s)", avgWait, 1000000.0 / avgWait);

    return 0;
}