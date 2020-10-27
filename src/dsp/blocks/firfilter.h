#pragma once
#include <dsp/block.h>

namespace dsp {

    void do_fir(complex_t* start, float* taps, complex_t* out, int dataCount, int tapCount) {
        int offset = tapCount - 1;
        for (int i = 0; i < dataCount; i++) {
            volk_32fc_32f_dot_prod_32fc((lv_32fc_t*)&out[i], (lv_32fc_t*)&start[i - offset], taps, tapCount);
        }
    }

    int blackman_window(float** taps, float sampleRate, float cutoff, float transWidth) {
        // Calculate tap count
        float fc = cutoff / sampleRate;
        if (fc > 1.0f) {
            fc = 1.0f;
        }
        int tapCount = 4.0f / (transWidth / sampleRate);
        if (tapCount < 4) {
            tapCount = 4;
        }
        if (tapCount % 2 == 0) { tapCount++; }

        // Allocate taps
        *taps = (float*)volk_malloc(tapCount * sizeof(float), volk_get_alignment());
        float* _taps = (float*)malloc(tapCount * sizeof(float));

        // Calculate and normalize taps
        float sum = 0.0f;
        float val;
        for (int i = 0; i < tapCount; i++) {
            val = (sin(2.0f * FL_M_PI * fc * ((float)i - (tapCount / 2))) / ((float)i - (tapCount / 2))) * 
                (0.42f - (0.5f * cos(2.0f * FL_M_PI / tapCount)) + (0.8f * cos(4.0f * FL_M_PI / tapCount)));
            _taps[i] = val;
            sum += val;
        }
        for (int i = 0; i < tapCount; i++) {
            _taps[i] /= sum;
        }

        // Flip taps
        float* flipped = *taps;
        for (int i = 0; i < tapCount; i++) {
            flipped[i] = _taps[tapCount - i - 1];
        }

        free(_taps);

        return tapCount;
    }

    class FIRFilter : public generic_block<FIRFilter> {
    public:
        FIRFilter() {}

        FIRFilter(stream<complex_t>* in, float sampleRate, float cutoff, float transWidth) { init(in, sampleRate, cutoff, transWidth); }

        ~FIRFilter() { generic_block<FIRFilter>::stop(); }

        void init(stream<complex_t>* in, float sampleRate, float cutoff, float transWidth) {
            _in = in;
            _sampleRate = sampleRate;
            _cutoff = cutoff;
            _transWidth = _transWidth;
            tapCount = blackman_window(&taps, sampleRate, cutoff, transWidth);
            buffer = (complex_t*)volk_malloc(STREAM_BUFFER_SIZE * sizeof(complex_t) * 2, volk_get_alignment());
            bufStart = &buffer[tapCount];
            generic_block<FIRFilter>::registerInput(_in);
            generic_block<FIRFilter>::registerOutput(&out);
        }

        void setInput(stream<complex_t>* in) {
            std::lock_guard<std::mutex> lck(generic_block<FIRFilter>::ctrlMtx);
            generic_block<FIRFilter>::tempStop();
            _in = in;
            generic_block<FIRFilter>::tempStart();
        }

        void setSampleRate(float sampleRate) {
            std::lock_guard<std::mutex> lck(generic_block<FIRFilter>::ctrlMtx);
            generic_block<FIRFilter>::tempStop();
            _sampleRate = sampleRate;
            updateTaps();
            generic_block<FIRFilter>::tempStart();
        }

        float getSampleRate() {
            return _sampleRate;
        }

        void setCutoff(float cutoff) {
            std::lock_guard<std::mutex> lck(generic_block<FIRFilter>::ctrlMtx);
            generic_block<FIRFilter>::tempStop();
            _cutoff = cutoff;
            updateTaps();
            generic_block<FIRFilter>::tempStart();
        }

        float getCutoff() {
            return _cutoff;
        }

        void setTransWidth(float transWidth) {
            std::lock_guard<std::mutex> lck(generic_block<FIRFilter>::ctrlMtx);
            generic_block<FIRFilter>::tempStop();
            _transWidth = transWidth;
            updateTaps();
            generic_block<FIRFilter>::tempStart();
        }

        float getTransWidth() {
            return _transWidth;
        }

        int run() {
            count = _in->read();
            if (count < 0) { return -1; }

            memcpy(bufStart, _in->data, count * sizeof(complex_t));
            _in->flush();

            // Write to output
            out.aquire();
            do_fir(bufStart, taps, out.data, count, tapCount);
            out.write(count);

            memmove(buffer, &buffer[count], tapCount);

            return count;
        }

        stream<complex_t> out;

    private:
        void updateTaps() {
            volk_free(taps);
            tapCount = blackman_window(&taps, _sampleRate, _cutoff, _transWidth);
            bufStart = &buffer[tapCount];
        }

        int count;
        float _sampleRate, _cutoff, _transWidth;
        stream<complex_t>* _in;

        complex_t* bufStart;
        complex_t* buffer;
        int tapCount;
        float* taps;

    };
}
