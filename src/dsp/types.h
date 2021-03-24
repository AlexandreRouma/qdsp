#pragma once

namespace dsp {
    struct complex_t {
        complex_t& operator*(const complex_t& b) {
            return complex_t{(i*b.i) - (q*b.q), (q*b.i) + (i*b.q)};
        }

        complex_t& operator+(const complex_t& b) {
            return complex_t{i+b.i, q+b.q};
        }

        float q;
        float i;
    };

    struct stereo_t {
        float l;
        float r;
    };
}