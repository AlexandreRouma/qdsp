#pragma once
#include <dsp/block.h>

namespace dsp {
    class AMDemod : public generic_block<AMDemod> {
    public:
        AMDemod() {}

        AMDemod(stream<complex_t>* in) { init(in); }

        ~AMDemod() { generic_block<AMDemod>::stop(); }

        void init(stream<complex_t>* in) {
            _in = in;
            generic_block<AMDemod>::registerInput(_in);
            generic_block<AMDemod>::registerOutput(&out);
        }

        void setInput(stream<complex_t>* in) {
            std::lock_guard<std::mutex> lck(generic_block<AMDemod>::ctrlMtx);
            generic_block<AMDemod>::tempStop();
            _in = in;
            generic_block<AMDemod>::tempStart();
        }

        int run() {
            count = _in->read();
            if (count < 0) { return -1; }

            out.aquire();
            volk_32fc_magnitude_32f(out.data, (lv_32fc_t*)_in->data, count);

            _in->flush();
            out.write(count);
            return count;
        }

        stream<float> out;

    private:
        int count;
        stream<complex_t>* _in;

    };
}
