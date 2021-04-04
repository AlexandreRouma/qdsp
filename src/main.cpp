#include <stdio.h>
#include <thread>
#include <chrono>
#include <iostream>
#include <random>
#include <limits>

#include <dsp/pll.h>
#include <dsp/stream.h>
#include <dsp/demodulator.h>
#include <dsp/window.h>
#include <dsp/resampling.h>
#include <dsp/processing.h>
#include <dsp/routing.h>
#include <dsp/source.h>
#include <dsp/sink.h>

#include <dsp/noaa/hrpt.h>
#include <dsp/noaa/tip.h>

std::ifstream inputFile("D:/basebands/n19felix.raw16", std::ios::binary);

dsp::stream<uint8_t> packedIn;
dsp::noaa::HRPTDemux demux(&packedIn);

dsp::noaa::TIPDemux tipDemux(&demux.TIPOut);

dsp::noaa::HIRSDemux hirsDemux(&tipDemux.HIRSOut);

dsp::FileSink<uint8_t> aipSink(&demux.AIPOut, "dumps/aip.bin");
dsp::FileSink<uint16_t> avhrr1Sink(&demux.AVHRRChan1Out, "dumps/avhrr1.bin");
dsp::FileSink<uint16_t> avhrr2Sink(&demux.AVHRRChan2Out, "dumps/avhrr2.bin");
dsp::FileSink<uint16_t> avhrr3Sink(&demux.AVHRRChan3Out, "dumps/avhrr3.bin");
dsp::FileSink<uint16_t> avhrr4Sink(&demux.AVHRRChan4Out, "dumps/avhrr4.bin");
dsp::FileSink<uint16_t> avhrr5Sink(&demux.AVHRRChan5Out, "dumps/avhrr5.bin");

dsp::FileSink<uint8_t> semSink(&tipDemux.SEMOut, "dumps/tip_sem.bin");
dsp::FileSink<uint8_t> dcsSink(&tipDemux.DCSOut, "dumps/tip_dcs.bin");
dsp::FileSink<uint8_t> sbuvSink(&tipDemux.SBUVOut, "dumps/tip_sbuv.bin");

// 20 output channels
dsp::FileSink<uint16_t> hirs1Sink(&hirsDemux.radChannels[0], "dumps/tip_hirs01.bin");
dsp::FileSink<uint16_t> hirs2Sink(&hirsDemux.radChannels[1], "dumps/tip_hirs02.bin");
dsp::FileSink<uint16_t> hirs3Sink(&hirsDemux.radChannels[2], "dumps/tip_hirs03.bin");
dsp::FileSink<uint16_t> hirs4Sink(&hirsDemux.radChannels[3], "dumps/tip_hirs04.bin");
dsp::FileSink<uint16_t> hirs5Sink(&hirsDemux.radChannels[4], "dumps/tip_hirs05.bin");
dsp::FileSink<uint16_t> hirs6Sink(&hirsDemux.radChannels[5], "dumps/tip_hirs06.bin");
dsp::FileSink<uint16_t> hirs7Sink(&hirsDemux.radChannels[6], "dumps/tip_hirs07.bin");
dsp::FileSink<uint16_t> hirs8Sink(&hirsDemux.radChannels[7], "dumps/tip_hirs08.bin");
dsp::FileSink<uint16_t> hirs9Sink(&hirsDemux.radChannels[8], "dumps/tip_hirs09.bin");
dsp::FileSink<uint16_t> hirs10Sink(&hirsDemux.radChannels[9], "dumps/tip_hirs10.bin");
dsp::FileSink<uint16_t> hirs11Sink(&hirsDemux.radChannels[10], "dumps/tip_hirs11.bin");
dsp::FileSink<uint16_t> hirs12Sink(&hirsDemux.radChannels[11], "dumps/tip_hirs12.bin");
dsp::FileSink<uint16_t> hirs13Sink(&hirsDemux.radChannels[12], "dumps/tip_hirs13.bin");
dsp::FileSink<uint16_t> hirs14Sink(&hirsDemux.radChannels[13], "dumps/tip_hirs14.bin");
dsp::FileSink<uint16_t> hirs15Sink(&hirsDemux.radChannels[14], "dumps/tip_hirs15.bin");
dsp::FileSink<uint16_t> hirs16Sink(&hirsDemux.radChannels[15], "dumps/tip_hirs16.bin");
dsp::FileSink<uint16_t> hirs17Sink(&hirsDemux.radChannels[16], "dumps/tip_hirs17.bin");
dsp::FileSink<uint16_t> hirs18Sink(&hirsDemux.radChannels[17], "dumps/tip_hirs18.bin");
dsp::FileSink<uint16_t> hirs19Sink(&hirsDemux.radChannels[18], "dumps/tip_hirs19.bin");
dsp::FileSink<uint16_t> hirs20Sink(&hirsDemux.radChannels[19], "dumps/tip_hirs20.bin");

void inWorker() {
    uint16_t* buffer = new uint16_t[11090];
    inputFile.ignore(4000000000); // 4GB max
    size_t length = inputFile.gcount();
    inputFile.clear();
    inputFile.seekg( 0, std::ios_base::beg );
    for (int J = 0; J < length; J += 11090 * sizeof(uint16_t)) {
        inputFile.read((char*)buffer, 11090 * sizeof(uint16_t));

        // Convert to our bit type
        for (int i = 0; i < 110900; i++) {
            if (i % 8 == 0) { packedIn.writeBuf[i / 8] = 0; }
            packedIn.writeBuf[i / 8] |= ((buffer[i / 10] >> (9 - (i % 10))) & 1) << (7 - (i % 8));
 
        }
        packedIn.swap(13863);
    }
    printf("Done!\n");
}

int main() {
    demux.start();
    tipDemux.start();
    hirsDemux.start();

    // HRPT Sinks
    aipSink.start();
    avhrr1Sink.start();
    avhrr2Sink.start();
    avhrr3Sink.start();
    avhrr4Sink.start();
    avhrr5Sink.start();

    // TIP Sinks
    semSink.start();
    dcsSink.start();
    sbuvSink.start();

    // HIRS Sinks
    hirs1Sink.start();
    hirs2Sink.start();
    hirs3Sink.start();
    hirs4Sink.start();
    hirs5Sink.start();
    hirs6Sink.start();
    hirs7Sink.start();
    hirs8Sink.start();
    hirs9Sink.start();
    hirs10Sink.start();
    hirs11Sink.start();
    hirs12Sink.start();
    hirs13Sink.start();
    hirs14Sink.start();
    hirs15Sink.start();
    hirs16Sink.start();
    hirs17Sink.start();
    hirs18Sink.start();
    hirs19Sink.start();
    hirs20Sink.start();

    std::thread worker(inWorker);

    printf("Started\n");

    while(1);

    return 0;
}