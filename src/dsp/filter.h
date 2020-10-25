#pragma once
#include <block.h>

namespace dsp {
    class PolyphaseFIR : public generic_block<PolyphaseFIR> {
    public:
        PolyphaseFIR() {}

        PolyphaseFIR(stream<complex_t>* in) { init(in); }

        ~PolyphaseFIR() { generic_block<PolyphaseFIR>::stop(); }

        void init(stream<complex_t>* in) {
            _in = in;
            generic_block<PolyphaseFIR>::registerInput(_in);
            generic_block<PolyphaseFIR>::registerOutput(&out);
        }

        void setInput(stream<complex_t>* in) {
            std::lock_guard<std::mutex> lck(generic_block<PolyphaseFIR>::ctrlMtx);
            generic_block<PolyphaseFIR>::tempStop();
            _in = in;
            generic_block<PolyphaseFIR>::tempStart();
        }

        int run() {
            count = _in->read();
            if (count < 0) { return -1; }

            out.aquire();
            

            _in->flush();
            out.write(count);
            return count;
        }

        stream<complex_t> out;

    private:
        int count;
        stream<complex_t>* _in;

    };
}