#pragma once

namespace dsp {
    struct complex_t {
        float q; //Quadrature(imaginary)
        float i; //In-Phase(real)
    };

    struct stereo_t {
        float l;
        float r;
    };
}
