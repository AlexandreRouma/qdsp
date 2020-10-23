#pragma once
#include <dsp/block.h>

namespace dsp {
    template <class T>
    class Splitter : public generic_block<Splitter<T>> {
    public:
        Splitter() {}

        Splitter(stream<T>* a, stream<T>* b) { init(a, b); }

        ~Splitter() { stop(); }

        void init(stream<T>* a, stream<T>* b) {
            _a = a;
            _b = b;
            registerInput(a);
            registerInput(b);
            registerOutput(&out);
        }

        stream<T> out;

    private:
        void worker() {
            while (true) {
                
            }
        }

        stream<T>* _a;
        stream<T>* _b;

    };
}