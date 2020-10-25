#pragma once
#include <dsp/block.h>

#define FL_M_PI                3.1415926535f
#define FAST_ATAN2_COEF1    FL_M_PI / 4.0f
#define FAST_ATAN2_COEF2    3.0f * FAST_ATAN2_COEF1

inline float fast_arctan2(float y, float x) {
    float abs_y = fabsf(y);
    float r, angle;
    if (x == 0.0f && y == 0.0f) { return 0.0f; }
    if (x>=0.0f) {
        r = (x - abs_y) / (x + abs_y);
        angle = FAST_ATAN2_COEF1 - FAST_ATAN2_COEF1 * r;
    }
    else {
        r = (x + abs_y) / (abs_y - x);
        angle = FAST_ATAN2_COEF2 - FAST_ATAN2_COEF1 * r;
    }
    if (y < 0.0f) {
        return -angle;
    }
   return angle;
}

namespace dsp {
    class FMDemod : public generic_block<FMDemod> {
    public:
        FMDemod() {}

        FMDemod(stream<complex_t>* in, float sampleRate, float deviation) { init(in, sampleRate, deviation); }

        ~FMDemod() { generic_block<FMDemod>::stop(); }

        void init(stream<complex_t>* in, float sampleRate, float deviation) {
            _in = in;
            _sampleRate = sampleRate;
            _deviation = deviation;
            generic_block<FMDemod>::registerInput(_in);
            generic_block<FMDemod>::registerOutput(&out);
        }

        void setInput(stream<complex_t>* in) {
            std::lock_guard<std::mutex> lck(generic_block<FMDemod>::ctrlMtx);
            generic_block<FMDemod>::tempStop();
            _in = in;
            generic_block<FMDemod>::tempStart();
        }

        void setSampleRate(float sampleRate) {
            std::lock_guard<std::mutex> lck(generic_block<FMDemod>::ctrlMtx);
            generic_block<FMDemod>::tempStop();
            _sampleRate = sampleRate;
            phasorSpeed = (_sampleRate / _deviation) / (2 * FL_M_PI);
            generic_block<FMDemod>::tempStart();
        }

        float getSampleRate() {
            return _sampleRate;
        }

        void setDeviation(float deviation) {
            std::lock_guard<std::mutex> lck(generic_block<FMDemod>::ctrlMtx);
            generic_block<FMDemod>::tempStop();
            _deviation = deviation;
            phasorSpeed = (_sampleRate / _deviation) / (2 * FL_M_PI);
            generic_block<FMDemod>::tempStart();
        }

        float getDeviation() {
            return _deviation;
        }

        int run() {
            count = _in->read();
            if (count < 0) { return -1; }

            // This is somehow faster than volk...

            float diff, currentPhase;

            out.aquire();
            for (int i = 0; i < count; i++) {
                currentPhase = fast_arctan2(_in->data[i].i, _in->data[i].q);
                diff = currentPhase - phase;
                if (diff > FL_M_PI)        { out.data[i] = (diff - 2 * FL_M_PI) * phasorSpeed; }
                else if (diff <= -FL_M_PI) { out.data[i] = (diff + 2 * FL_M_PI) * phasorSpeed; }
                phase = currentPhase;
            }

            _in->flush();
            out.write(count);
            return count;
        }

        stream<float> out;

    private:
        int count;
        float phase, phasorSpeed, _sampleRate, _deviation;
        stream<complex_t>* _in;

    };

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