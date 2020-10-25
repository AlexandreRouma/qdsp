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

    template <int ORDER>
    class PolyphaseFIR : public generic_block<PolyphaseFIR<ORDER>> {
    public:
        PolyphaseFIR() {}

        PolyphaseFIR(stream<complex_t>* in, float sampleRate, float cutoff, float transWidth) { init(in, sampleRate, cutoff, transWidth); }

        ~PolyphaseFIR() { generic_block<PolyphaseFIR<ORDER>>::stop(); }

        void init(stream<complex_t>* in, float sampleRate, float cutoff, float transWidth) {
            _in = in;
            _sampleRate = sampleRate;
            _cutoff = cutoff;
            _transWidth = _transWidth;
            tapCount = blackman_window(&fullTaps, sampleRate, cutoff, transWidth);

            tapCountLow = tapCount / ORDER;
            tapCountHigh = tapCountLow + 1;
            tapSwitchPoint = tapCount % ORDER;

            for (int i = 0; i < ORDER; i++) {
                buffer[i] = (complex_t*)volk_malloc((STREAM_BUFFER_SIZE / ORDER) * sizeof(complex_t) * 2, volk_get_alignment());
                outBuffer[i] = (complex_t*)volk_malloc((STREAM_BUFFER_SIZE / ORDER) * sizeof(complex_t) * 2, volk_get_alignment());
                taps[i] = (float*)volk_malloc(tapCountHigh * sizeof(float), volk_get_alignment());
            }

            generic_block<PolyphaseFIR<ORDER>>::registerInput(_in);
            generic_block<PolyphaseFIR<ORDER>>::registerOutput(&out);
        }

        void setInput(stream<complex_t>* in) {
            std::lock_guard<std::mutex> lck(generic_block<PolyphaseFIR<ORDER>>::ctrlMtx);
            generic_block<PolyphaseFIR<ORDER>>::tempStop();
            _in = in;
            generic_block<PolyphaseFIR<ORDER>>::tempStart();
        }

        int run() {
            count = _in->read();
            if (count < 0) { return -1; }

            int lowCount = count / ORDER;
            int highCount = lowCount + 1;
            int switchPoint = count % ORDER;

            // Read data into delay buffers
            for (int i = 0; i < count; i++) {
                buffer[i % ORDER][(i / ORDER) + tapCount] = _in->data[i];
            }
            _in->flush();

            // Process through FIR
            for (int i = 0; i < ORDER; i++) {
                if (i < switchPoint) {
                    if (i < tapSwitchPoint) {
                        do_fir(&buffer[i][tapCount], taps[i], outBuffer[i], highCount, tapCountHigh);
                    }
                    else {
                        do_fir(&buffer[i][tapCount], taps[i], outBuffer[i], highCount, tapCountLow);
                    }
                }
                else {
                    if (i < tapSwitchPoint) {
                        do_fir(&buffer[i][tapCount], taps[i], outBuffer[i], lowCount, tapCountHigh);
                    }
                    else {
                        do_fir(&buffer[i][tapCount], taps[i], outBuffer[i], lowCount, tapCountLow);
                    }
                }
            }

            // Save end of the data
            for (int i = 0; i < ORDER; i++) {
                if (i < switchPoint) {
                    memmove(buffer[i], &buffer[i][highCount], tapCount * sizeof(complex_t));
                }
                else {
                    memmove(buffer[i], &buffer[i][lowCount], tapCount * sizeof(complex_t));
                }
            }

            // Write to output
            out.aquire();
            for (int i = 0; i < count; i++) {
                out.data[i] = outBuffer[i % ORDER][(i / ORDER)];
            }
            out.write(count);

            return count;
        }

        stream<complex_t> out;

    private:
        int count;
        float _sampleRate, _cutoff, _transWidth;
        stream<complex_t>* _in;

        complex_t* buffer[ORDER];
        complex_t* outBuffer[ORDER];
        float* fullTaps;
        float* taps[ORDER];

        int tapCount;
        int tapCountLow;
        int tapCountHigh;
        int tapSwitchPoint;

    };
}