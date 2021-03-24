#pragma once
#include <dsp/block.h>
#include <dsp/interpolation_taps.h>
#include <math.h>

#define DSP_SIGN(n)     ((n) >= 0)
#define DSP_STEP(n)     (((n) > 0.0f) ? 1.0f : -1.0f)

namespace dsp {
    class SqSymbolRecovery : public generic_block<SqSymbolRecovery> {
    public:
        SqSymbolRecovery() {}

        SqSymbolRecovery(stream<float>* in, int omega) { init(in, omega); }

        ~SqSymbolRecovery() {
            generic_block<SqSymbolRecovery>::stop();
        }

        void init(stream<float>* in, int omega) {
            _in = in;
            samplesPerSymbol = omega;
            generic_block<SqSymbolRecovery>::registerInput(_in);
            generic_block<SqSymbolRecovery>::registerOutput(&out);
        }

        int run() {
            count = _in->read();
            if (count < 0) { return -1; }

            int outCount = 0;

            for (int i = 0; i < count; i++) {
                if (DSP_SIGN(lastVal) != DSP_SIGN(_in->readBuf[i])) {
                    counter = samplesPerSymbol / 2;
                    lastVal = _in->readBuf[i];
                    continue;
                }

                if (counter >= samplesPerSymbol) {
                    counter = 0;
                    out.writeBuf[outCount] = _in->readBuf[i];
                    outCount++;
                }
                else {
                    counter++;
                }

                lastVal = _in->readBuf[i];
            }
            
            _in->flush();
            if (!out.swap(outCount)) { return -1; }
            return count;
        }

        stream<float> out;

    private:
        int count;
        int samplesPerSymbol = 1;
        int counter = 0;
        float lastVal = 0;
        stream<float>* _in;

    };



    class MMClockRecovery : public generic_block<MMClockRecovery> {
    public:
        MMClockRecovery() {}

        MMClockRecovery(stream<float>* in, float omega, float gainOmega, float muGain, float omegaRelLimit) {
            init(in, omega, gainOmega, muGain, omegaRelLimit);
        }

        ~MMClockRecovery() {
            generic_block<MMClockRecovery>::stop();
        }

        void init(stream<float>* in, float omega, float gainOmega, float muGain, float omegaRelLimit) {
            _in = in;
            _omega = omega;
            _muGain = muGain;
            _gainOmega = gainOmega;
            _omegaRelLimit = omegaRelLimit;

            omegaMin = _omega - (_omega * _omegaRelLimit);
            omegaMax = _omega + (_omega * _omegaRelLimit);
            _dynOmega = _omega;

            generic_block<MMClockRecovery>::registerInput(_in);
            generic_block<MMClockRecovery>::registerOutput(&out);
        }

        int run() {
            count = _in->read();
            if (count < 0) { return -1; }

            int outCount = 0;
            float outVal;
            float phaseError;
            float roundedStep;
            int maxOut = 2.0f * _omega * (float)count;

            // Copy the first 7 values to the delay buffer for fast computing
            memcpy(&delay[7], _in->readBuf, 7 * sizeof(float));

            int i = nextOffset;
            for (; i < count && outCount < maxOut;) {
                // Calculate output value
                // If we still need to use the old values, calculate using delay buf
                // Otherwise, use normal buffer
                if (i < 7) {
                    volk_32f_x2_dot_prod_32f(&outVal, &delay[i], INTERP_TAPS[(int)roundf(_mu * 128.0f)], 8);
                }
                else {
                    volk_32f_x2_dot_prod_32f(&outVal, &_in->readBuf[i - 7], INTERP_TAPS[(int)roundf(_mu * 128.0f)], 8);
                }
                out.writeBuf[outCount] = outVal;

                // Cursed phase detect approximation (don't ask me how this approximation works)
                phaseError = (DSP_STEP(lastOutput)*outVal) - (lastOutput*DSP_STEP(outVal));
                lastOutput = outVal;
                outCount++;

                // Clamp phase error
                if (phaseError > 1.0f) { phaseError = 1.0f; }
                if (phaseError < -1.0f) { phaseError = -1.0f; }

                // Adjust the symbol rate using the phase error approximation and clamp
                // TODO: Branchless clamp
                _dynOmega = _dynOmega + (_gainOmega * phaseError);
                if (_dynOmega > omegaMax) { _dynOmega = omegaMax; }
                else if (_dynOmega < omegaMin) { _dynOmega = omegaMin; }

                // Adjust the symbol phase according to the phase error approximation
                // It will now contain the phase delta needed to jump to the next symbol
                // Rounded step will contain the rounded number of symbols
                _mu = _mu + _dynOmega + (_muGain * phaseError);
                roundedStep = floor(_mu);

                // Step to where the next symbol should be
                i += (int)roundedStep;

                // Now that we've stepped to the next symbol, keep only the offset inside the symbol
                _mu -= roundedStep;
            }

            nextOffset = i - count;

            // Save the last 7 values for the next round
            memcpy(delay, &_in->readBuf[count - 7], 7 * sizeof(float));
            
            _in->flush();
            if (!out.swap(outCount)) { return -1; }
            return count;
        }

        stream<float> out;

    private:
        int count;

        // Delay buffer
        float delay[15];
        int nextOffset = 0;

        // Configuration
        float _omega = 1.0f;
        float _muGain = 1.0f;
        float _gainOmega = 0.001f;
        float _omegaRelLimit = 0.005;

        // Precalculated values
        float omegaMin = _omega + (_omega * _omegaRelLimit);
        float omegaMax = _omega + (_omega * _omegaRelLimit);

        // Runtime adjusted
        float _dynOmega = _omega;
        float _mu = 0.5f;
        float lastOutput = 0.0f;
        

        stream<float>* _in;

    };

    class FourthOrderCostas : public generic_block<FourthOrderCostas> {
    public:
        FourthOrderCostas() {}

        FourthOrderCostas(stream<complex_t>* in, float loopBandwidth) { init(in, loopBandwidth); }

        ~FourthOrderCostas() {
            generic_block<FourthOrderCostas>::stop();
        }

        void init(stream<complex_t>* in, float loopBandwidth) {
            _in = in;
            lastVCO.i = 1.0f;
            lastVCO.q = 0.0f;
            _loopBandwidth = loopBandwidth;

            float dampningFactor = sqrtf(2.0f) / 2.0f;
            float denominator = (1.0 + 2.0 * dampningFactor * _loopBandwidth + _loopBandwidth * _loopBandwidth);
            _alpha = (4 * dampningFactor * _loopBandwidth) / denominator;
            _beta = (4 * _loopBandwidth * _loopBandwidth) / denominator;

            generic_block<FourthOrderCostas>::registerInput(_in);
            generic_block<FourthOrderCostas>::registerOutput(&out);
        }

        int run() {
            count = _in->read();
            if (count < 0) { return -1; }

            complex_t outVal;
            float error;

            for (int i = 0; i < count; i++) {

                // Mix the VFO with the input to create the output value
                outVal = lastVCO * _in->readBuf[i];
                out.writeBuf[i] = outVal;

                // Calculate the phase error
                error = (DSP_STEP(outVal.i) * outVal.q) - (DSP_STEP(outVal.q) * outVal.i);
                if (error > 1.0f) { error = 1.0f; }
                if (error < -1.0f) { error = -1.0f; }
                
                // Integrate frequency and clamp it
                vcoFrequency += _beta * error;
                if (vcoFrequency > 1.0f) { vcoFrequency = 1.0f; }
                if (vcoFrequency < -1.0f) { vcoFrequency = -1.0f; }

                // Calculate new phase and wrap it
                vcoPhase += vcoFrequency + (_alpha * error);
                while (vcoPhase > (2.0f * FL_M_PI)) { vcoPhase -= (2.0f * FL_M_PI); }
                while (vcoPhase < (-2.0f * FL_M_PI)) { vcoPhase += (2.0f * FL_M_PI); }

                // Calculate output
                lastVCO.i = cosf(vcoPhase);
                lastVCO.q = sinf(vcoPhase);

            }
            
            _in->flush();
            if (!out.swap(count)) { return -1; }
            return count;
        }

        stream<complex_t> out;

    private:
        int count;
        float _loopBandwidth = 1.0f;

        float _alpha; // Integral coefficient
        float _beta; // Proportional coefficient
        float vcoFrequency = 0.0f;
        float vcoPhase = 0.0f;
        complex_t lastVCO;

        stream<complex_t>* _in;

    };
}