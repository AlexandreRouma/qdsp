#pragma once
#include <dsp/block.h>
#include <volk/volk.h>

namespace dsp {
    template <class T>
    class Add : public generic_block<Add<T>> {
    public:
        Add() {}

        Add(stream<T>* a, stream<T>* b) { init(a, b); }

        ~Add() { stop(); }

        void init(stream<T>* a, stream<T>* b) {
            _a = a;
            _b = b;
            registerInput(a);
            registerInput(b);
            registerOutput(&out);
        }

        int run() {
            a_count = _a->read();
            b_count = _b->read();
            if (a_count < 0 || b_count < 0) { return -1; }
            if (a_count != b_count) {
                _a->flush();
                _b->flush();
                return 0;
            }

            if constexpr (std::is_same_v<T, complex_t> || std::is_same_v<T, stereo_t>) {
                volk_32fc_x2_add_32fc(_a->data, _b->data, out.data, a_count);
            }
            else {
                volk_32f_x2_add_32f(_a->data, _b->data, out.data, a_count);
            }

            _a->flush();
            _b->flush();
            out.write(a_count);
        }

        stream<T> out;

    private:
        int a_count, b_count;
        stream<T>* _a;
        stream<T>* _b;

    };

    template <class T>
    class Multiply : public generic_block<Multiply<T>> {
    public:
        Multiply() {}

        Multiply(stream<T>* a, stream<T>* b) { init(a, b); }

        ~Multiply() { stop(); }

        void init(stream<T>* a, stream<T>* b) {
            _a = a;
            _b = b;
            registerInput(a);
            registerInput(b);
            registerOutput(&out);
        }

        int run() {
            a_count = _a->read();
            b_count = _b->read();
            if (a_count < 0 || b_count < 0) { return -1; }
            if (a_count != b_count) {
                _a->flush();
                _b->flush();
                return 0;
            }

            if constexpr (std::is_same_v<T, complex_t>) {
                volk_32fc_x2_multiply_32fc(_a->data, _b->data, out.data, a_count);
            }
            else {
                volk_32f_x2_multiply_32f(_a->data, _b->data, out.data, a_count);
            }

            _a->flush();
            _b->flush();
            out.write(a_count);
            return a_count;
        }

        stream<T> out;

    private:
        int a_count, b_count;
        stream<T>* _a;
        stream<T>* _b;

    };
}