#include <stdio.h>
#include <dsp/clock_recovery.h>

/*
    Standard throughput testbench
*/

#define BLOCK_SIZE      1000000

//dsp::stream<float> input;
//dsp::stream<dsp::complex_t> input;
dsp::stream<dsp::complex_t> input;

// ================ DUT ================
dsp::MMClockRecovery<dsp::complex_t> dut(&input, 2.33f, powf(0.01f, 2) / 4.0f, 0.01f, 100e-6f);
// =====================================

// Counter
uint64_t samples = 0;

// Writer thread
void writeWorker() {
    // Create buffer
    // float* buffer = new float[STREAM_BUFFER_SIZE];
    // for (int i = 0; i < BLOCK_SIZE; i++) {
    //     buffer[i] = ((static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) * 2.0f) - 1;
    // }

    dsp::complex_t* buffer = new dsp::complex_t[STREAM_BUFFER_SIZE];
    for (int i = 0; i < BLOCK_SIZE; i++) {
        buffer[i].re = ((static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) * 2.0f) - 1;
        buffer[i].im = ((static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) * 2.0f) - 1;
    }

    // dsp::stereo_t* buffer = new dsp::stereo_t[STREAM_BUFFER_SIZE];
    // for (int i = 0; i < BLOCK_SIZE; i++) {
    //     buffer[i].l = ((static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) * 2.0f) - 1;
    //     buffer[i].r = ((static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) * 2.0f) - 1;
    // }

    // Write
    while (true) {
        memcpy(input.writeBuf, buffer, BLOCK_SIZE * sizeof(dsp::complex_t));
        input.swap(BLOCK_SIZE);
    }
}

void readWorker() {
    while (true) {
        samples += dut.out.read();
        dut.out.flush();

        // samples += dut.out_left.read();
        // dut.out_right.read();
        // dut.out_left.flush();
        // dut.out_right.flush();
    }
}

int main() {
    // Start DUT
    dut.start();

    // Start workers
    std::thread writer(writeWorker);
    std::thread reader(readWorker);

    // Run test
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
        printf("%" PRIu64 " S/s\n", samples / 10);
        samples = 0;
    }

    return 0;
}