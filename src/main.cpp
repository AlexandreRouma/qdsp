
#include <stdio.h>
#include <thread>
#include <chrono>
#include <iostream>
#include <random>
#include <spdlog/spdlog.h>

#include <dsp/convertion.h>
#include <dsp/processing.h>
#include <dsp/resampling.h>
#include <dsp/sink.h>

#define SAMPLE_COUNT    1000000
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
    dsp::RealToComplex r2c(&inputStream);
    dsp::FrequencyXlator<dsp::complex_t> xlator(&r2c.out, 64000000, 14000000);
    dsp::PowerDecimator decim(&xlator.out, 3);
    dsp::NullSink<dsp::complex_t> ns(&decim.out);
    r2c.start();
    xlator.start();
    decim.start();
    ns.start();
    // ===================

    double totalWait = 0.0f;

    for (int i = 0; i < RUN_COUNT; i++) {
        auto start = std::chrono::high_resolution_clock::now();

        memcpy(inputStream.writeBuf, randBuf, SAMPLE_COUNT * sizeof(float));
        inputStream.swap(SAMPLE_COUNT);

        auto stop = std::chrono::high_resolution_clock::now();

        auto delay = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
        totalWait += delay.count();
    }

    double avgWait = totalWait / (double)RUN_COUNT;

    spdlog::info("Average run took {0} uS ({1} MS/s)", avgWait, 1000000.0 / avgWait);

    return 0;
}