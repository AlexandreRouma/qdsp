#pragma once
#include <dsp/block.h>
#include <volk/volk.h>
#include <random>

//Random Source block

namespace dsp {
    template <class T>
    class RandomSource : public source_block<RandomSource<T>> {
    public:
        RandomSource() { }

        ~RandomSource() { source_block<RandomSource>::stop(); }
        
        
        int run() {
            int ret = out.aquire();
            if(ret != 0)
                return ret; //If stream is closed
            int nitems = STREAM_BUFFER_SIZE / 200;
            for(int i = 0; i < nitems; i++) {
                if constexpr (std::is_same_v<T, complex_t>) {
                    out.data[i].q = float(rand())/float((RAND_MAX)) - 1.0f;
                    out.data[i].i = float(rand())/float((RAND_MAX)) - 1.0f;
                } else
                    out.data[i] = float(rand())/float((RAND_MAX)) - 1.0f;
            }
            out.write(nitems);
            return nitems;
        }

        stream<T> out;
    };
}
