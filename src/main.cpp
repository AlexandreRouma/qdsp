#include <dsp/deframing.h>
#include <dsp/source.h>
#include <dsp/sink.h>
#include <dsp/falcon_fec.h>
#include <dsp/falcon_packet.h>
#include <fstream>

std::ifstream inputFile("D:/falcon_out_good.bin", std::ios::binary);
std::ofstream outputFile("D:/falcon_video.ts", std::ios::binary);

uint8_t tempBuf[8192];
//int8_t tempBuf[8192];
uint64_t totalRead = 0;

int sourceHandler(uint8_t* data, void* ctx) {
    inputFile.read((char*)tempBuf, 1024);
    totalRead += 1024;

    if (totalRead >= 139000000) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        printf("Reached end of file\n");
        return -1;
    }

    for (int i = 0; i < 1024; i++) {
        data[i * 8] = ((tempBuf[i] >> 7) & 1);
        data[(i * 8) + 1] = ((tempBuf[i] >> 6) & 1);
        data[(i * 8) + 2] = ((tempBuf[i] >> 5) & 1);
        data[(i * 8) + 3] = ((tempBuf[i] >> 4) & 1);
        data[(i * 8) + 4] = ((tempBuf[i] >> 3) & 1);
        data[(i * 8) + 5] = ((tempBuf[i] >> 2) & 1);
        data[(i * 8) + 6] = ((tempBuf[i] >> 1) & 1);
        data[(i * 8) + 7] = (tempBuf[i] & 1);
    }

    // inputFile.read((char*)tempBuf, 8192);
    // totalRead += 8192;

    // for (int i = 0; i < 8192; i++) {
    //     data[i] = (tempBuf[i] > 0);
    // }

    return 8192;

    // inputFile.read((char*)data, 4);
    // inputFile.read((char*)data, 1275);
    // return 1024;
}

uint8_t seq[255];
void apply(uint8_t* data, int len) {
    for (int i = 0; i < len; i++) {
        data[i] ^= seq[i % 255];
    }
}

void sinkHandler(uint8_t* data, int count, void* ctx) {
    //outputFile.write("\x1a\xcf\xfc\x1d", 4);
    //outputFile.write((char*)data, count);

    uint16_t length = (((data[0] & 0b1111) << 8) | data[1]) + 2;
    uint64_t pktId =    ((uint64_t)data[2] << 56) | ((uint64_t)data[3] << 48) | ((uint64_t)data[4] << 40) | ((uint64_t)data[5] << 32)
                    |   ((uint64_t)data[6] << 24) | ((uint64_t)data[7] << 16) | ((uint64_t)data[8] << 8) | data[9];

    if (pktId != 0x01123201042E1403) { return; }

    printf("%016" PRIX64 ": %d bytes, %d full\n", pktId, length, count);

    //outputFile.write((char*)(data + 25), length - 25 - 4);

    //outputFile.write((char*)data, length);
    outputFile.write((char*)data, 1047);

    // for (int i = 0; i < 16; i++) { 
    //     printf("%02X ", data[i]);
    // }
    // printf("\n");
}

dsp::HandlerSource<uint8_t> source(sourceHandler, NULL);

// 0x1acffc1d
uint8_t syncWord[] = {0,0,0,1,1,0,1,0,1,1,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,1,1,1,0,1};
dsp::Deframer deframe(&source.out, 10232, syncWord, 32);
dsp::FalconRS falconRS(&deframe.out);
dsp::FalconPacketSync pkt(&falconRS.out);
dsp::HandlerSink<uint8_t> sink(&pkt.out, sinkHandler, NULL);

uint8_t sequenceGen(uint8_t x) {
    uint8_t lastBit = (x & 1);
    lastBit ^= ((x >> 3) & 1);
    lastBit ^= ((x >> 5) & 1);
    lastBit ^= ((x >> 7) & 1);
    x = (x >> 1) | (lastBit << 7);
    return x;
}

int main() {
    if (!inputFile.is_open()) { return 1; }
    if (!outputFile.is_open()) { return 1; }

    sink.start();
    pkt.start();
    falconRS.start();
    deframe.start();
    source.start();

    while (1) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }

    return 0;
}