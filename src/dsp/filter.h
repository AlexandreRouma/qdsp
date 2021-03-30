#pragma once
#include <dsp/block.h>
#include <dsp/window.h>
#include <string.h>

namespace dsp {

    template <class T>
    class FIR : public generic_block<FIR<T>> {
    public:
        FIR() {}

        FIR(stream<T>* in, dsp::filter_window::generic_window* window) { init(in, window); }

        ~FIR() {
            generic_block<FIR<T>>::stop();
            volk_free(buffer);
            volk_free(taps);
        }

        void init(stream<T>* in, dsp::filter_window::generic_window* window) {
            _in = in;

            tapCount = window->getTapCount();
            taps = (float*)volk_malloc(tapCount * sizeof(float), volk_get_alignment());
            window->createTaps(taps, tapCount);

            buffer = (T*)volk_malloc(STREAM_BUFFER_SIZE * sizeof(T) * 2, volk_get_alignment());
            bufStart = &buffer[tapCount];
            generic_block<FIR<T>>::registerInput(_in);
            generic_block<FIR<T>>::registerOutput(&out);
        }

        void setInput(stream<T>* in) {
            std::lock_guard<std::mutex> lck(generic_block<FIR<T>>::ctrlMtx);
            generic_block<FIR<T>>::tempStop();
            generic_block<FIR<T>>::unregisterInput(_in);
            _in = in;
            generic_block<FIR<T>>::registerInput(_in);
            generic_block<FIR<T>>::tempStart();
        }

        void updateWindow(dsp::filter_window::generic_window* window) {
            _window = window;
            volk_free(taps);
            tapCount = window->getTapCount();
            taps = (float*)volk_malloc(tapCount * sizeof(float), volk_get_alignment());
            window->createTaps(taps, tapCount);
        }

        int run() {
            int count = _in->read();
            if (count < 0) { return -1; }

            memcpy(bufStart, _in->readBuf, count * sizeof(T));
            _in->flush();

            if constexpr (std::is_same_v<T, float>) {
                for (int i = 0; i < count; i++) {
                    volk_32f_x2_dot_prod_32f((float*)&out.writeBuf[i], (float*)&buffer[i+1], taps, tapCount);
                }
            }
            if constexpr (std::is_same_v<T, complex_t>) {
                for (int i = 0; i < count; i++) {
                    volk_32fc_32f_dot_prod_32fc((lv_32fc_t*)&out.writeBuf[i], (lv_32fc_t*)&buffer[i+1], taps, tapCount);
                }
            }

            if (!out.swap(count)) { return -1; }

            memmove(buffer, &buffer[count], tapCount * sizeof(T));

            return count;
        }

        stream<T> out;

    private:
        stream<T>* _in;

        dsp::filter_window::generic_window* _window;

        T* bufStart;
        T* buffer;
        int tapCount;
        float* taps;

    };

    class BFMDeemp : public generic_block<BFMDeemp> {
    public:
        BFMDeemp() {}

        BFMDeemp(stream<stereo_t>* in, float sampleRate, float tau) { init(in, sampleRate, tau); }

        ~BFMDeemp() { generic_block<BFMDeemp>::stop(); }

        void init(stream<stereo_t>* in, float sampleRate, float tau) {
            _in = in;
            _sampleRate = sampleRate;
            _tau = tau;
            float dt = 1.0f / _sampleRate;
            alpha = dt / (_tau + dt);
            generic_block<BFMDeemp>::registerInput(_in);
            generic_block<BFMDeemp>::registerOutput(&out);
        }

        void setInput(stream<stereo_t>* in) {
            std::lock_guard<std::mutex> lck(generic_block<BFMDeemp>::ctrlMtx);
            generic_block<BFMDeemp>::tempStop();
            generic_block<BFMDeemp>::unregisterInput(_in);
            _in = in;
            generic_block<BFMDeemp>::registerInput(_in);
            generic_block<BFMDeemp>::tempStart();
        }

        void setSampleRate(float sampleRate) {
            _sampleRate = sampleRate;
            float dt = 1.0f / _sampleRate;
            alpha = dt / (_tau + dt);
        }

        void setTau(float tau) {
            _tau = tau;
            float dt = 1.0f / _sampleRate;
            alpha = dt / (_tau + dt);
        }

        int run() {
            count = _in->read();
            if (count < 0) { return -1; }

            if (bypass) {
                memcpy(out.writeBuf, _in->readBuf, count * sizeof(stereo_t));
                _in->flush();
                if (!out.swap(count)) { return -1; }
                return count;
            }

            if (isnan(lastOutL)) {
                lastOutL = 0.0f;
            }
            if (isnan(lastOutR)) {
                lastOutR = 0.0f;
            }
            out.writeBuf[0].l = (alpha * _in->readBuf[0].l) + ((1-alpha) * lastOutL);
            out.writeBuf[0].r = (alpha * _in->readBuf[0].r) + ((1-alpha) * lastOutR);
            for (int i = 1; i < count; i++) {
                out.writeBuf[i].l = (alpha * _in->readBuf[i].l) + ((1 - alpha) * out.writeBuf[i - 1].l);
                out.writeBuf[i].r = (alpha * _in->readBuf[i].r) + ((1 - alpha) * out.writeBuf[i - 1].r);
            }
            lastOutL = out.writeBuf[count - 1].l;
            lastOutR = out.writeBuf[count - 1].r;

            _in->flush();
            if (!out.swap(count)) { return -1; }
            return count;
        }

        bool bypass = false;

        stream<stereo_t> out;

    private:
        int count;
        float lastOutL = 0.0f;
        float lastOutR = 0.0f;
        float alpha;
        float _tau;
        float _sampleRate;
        stream<stereo_t>* _in;

    };
}