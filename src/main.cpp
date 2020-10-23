
#include <stdio.h>
#include <dsp/stream.h>
#include <dsp/block.h>
#include <dsp/math.h>
#include <thread>

int main() {
    printf("Hello World!\n");

    dsp::stream<float> a_stream;
    dsp::stream<float> b_stream;

    dsp::Add<float> adder(&a_stream, &b_stream);

    adder.start();

    a_stream.write(1000000);
    b_stream.write(1000000);

    // while(1) {
    //     // printf("Read %d\n", adder.out.read());
    //     // adder.out.flush();
    // }

    printf("Returning!\n");

    return 0;
}