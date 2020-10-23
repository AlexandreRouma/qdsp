
#include <stdio.h>
#include <dsp/stream.h>
#include <dsp/block.h>
#include <dsp/routing.h>
#include <dsp/math.h>
#include <thread>
#include <chrono>
#include <iostream>

int main() {
    printf("Hello World!\n");

    dsp::stream<float> a_stream;
    dsp::stream<float> b_stream;
    dsp::stream<float> out_stream;

    dsp::Add<float> adder(&a_stream, &b_stream);
    dsp::Splitter<float> split(&adder.out);

    split.bindStream(&out_stream);

    adder.start();
    split.start();

    

    while(1) {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 1000; i++) {
            a_stream.aquire();
            a_stream.write(1000000);
            b_stream.aquire();
            b_stream.write(1000000);
            out_stream.read();
            out_stream.flush();
        } 
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start); 
  
        std::cout << "Time taken by function: " << duration.count() << " microseconds" << std::endl; 
    }

    printf("Returning!\n");

    return 0;
}