#include <stdio.h>
#include <thread>
#include <chrono>
#include <iostream>
#include <random>
#include <spdlog/spdlog.h>
#include <wav.h>
#include <wavreader.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#include <dsp/pll.h>
#include <dsp/stream.h>
#include <dsp/demodulator.h>
#include <dsp/window.h>
#include <dsp/resampling.h>
#include <dsp/processing.h>
#include <dsp/routing.h>

#include <dsp/deframing.h>
#include <dsp/falcon_fec.h>
#include <dsp/falcon_packet.h>
#include <dsp/source.h>
#include <dsp/sink.h>

#include <dsp/utils/ccsds.h>

WavReader reader("D:/Downloads/baseband_10-20-29_22-03-2021_2247_5_one_minute.wav");
WavWriter writer("D:/starship_costas.wav", 16, 2, 5000000);

uint64_t totalRead = 0;

int sourceHandler(dsp::complex_t* data, void* ctx) {
    if (totalRead >= reader.hdr.dataSize) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        writer.close();
        printf("Done\n");
        while(1);
    }

    int16_t samples[1024 * 2];
    reader.readSamples(samples, 1024 * 2 * sizeof(int16_t));
    for (int i = 0; i < 1024; i++) {
        data[i].re = (float)samples[i * 2] / 32768.0f;
        data[i].im = (float)samples[(i * 2) + 1] / 32768.0f;
    }
    totalRead += 1024 * 2 * sizeof(int16_t);
    return 1024;
}

int16_t outSamples[STREAM_BUFFER_SIZE];

void sinkHandler(dsp::complex_t* data, int count, void* ctx) {
    for (int i = 0; i < count; i++) {
        outSamples[i * 2] = data[i].re * 8000.0f;
        outSamples[(i * 2) + 1] = data[i].im * 8000.0f;
    }
    writer.writeSamples(outSamples, count * 2 * sizeof(int16_t));
}


dsp::HandlerSource<dsp::complex_t> source(sourceHandler, NULL);
dsp::ComplexAGC agc(&source.out, 1.0f, 65536, 10e-4);
dsp::CostasLoop<4> costas(&agc.out, 0.01f);
dsp::DelayImag delay(&costas.out);
dsp::MMClockRecovery<dsp::complex_t> recov(&delay.out, 5000000.0f / 3125000.0f, (0.01 * 0.01) / 4, 0.01, 0.005);
dsp::HandlerSink<dsp::complex_t> sink(&recov.out, sinkHandler, NULL);

int main() {
    source.start();
    agc.start();
    costas.start();
    delay.start();
    recov.start();
    sink.start();

    printf("Started\n");

    while(1);

    return 0;
}