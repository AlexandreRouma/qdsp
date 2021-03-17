#pragma once
#include <dsp/block.h>

#define DSP_SIGN(n)     ((n) >= 0)
#define DSP_STEP(n)     (((n) > 0.0f) ? 1.0f : -1.0f)

namespace dsp {
    class SqSymbolRecovery : public generic_block<SqSymbolRecovery> {
    public:
        SqSymbolRecovery() {}

        SqSymbolRecovery(stream<float>* in, int sampPerSym) { init(in, sampPerSym); }

        ~SqSymbolRecovery() {
            generic_block<SqSymbolRecovery>::stop();
        }

        void init(stream<float>* in, int sampPerSym) {
            _in = in;
            samplesPerSymbol = sampPerSym;
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

        MMClockRecovery(stream<float>* in, float sampPerSym, float symPhaseGain, float symLenGain, float symRateDelta) {
            init(in, sampPerSym, symPhaseGain, symLenGain, symRateDelta);
        }

        ~MMClockRecovery() {
            generic_block<MMClockRecovery>::stop();
        }

        void init(stream<float>* in, float sampPerSym, float symPhaseGain, float symLenGain, float symRateDelta) {
            _in = in;
            _sampPerSym = sampPerSym;
            _symPhaseGain = symPhaseGain;
            _symLenGain = symLenGain;
            _symRateDelta = symRateDelta;

            SampPerSymMin = _sampPerSym + (_sampPerSym * _symRateDelta);
            SampPerSymMax = _sampPerSym + (_sampPerSym * _symRateDelta);
            dynSampPerSym = _sampPerSym;

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

            for (int i = 0; i < count - 1;) {
                // Calculate output value
                
                // TEMPORARY AND BAD APPROXIMATION FOR THE UNKNOWN SYMBOL
                outVal = (symPhase * _in->readBuf[i]) + ((1.0f - symPhase) * lastInput);
                lastInput = _in->readBuf[i];
                out.writeBuf[outCount] = outVal;

                // Cursed phase detect approximation (don't ask me how this approximation works)
                phaseError = (DSP_STEP(lastOutput)*outVal) - (lastOutput*DSP_STEP(outVal));
                lastOutput = outVal;
                outCount++;

                // Adjust the symbol rate using the phase error approximation and clamp
                // TODO: Branchless clamp
                dynSampPerSym = dynSampPerSym + (_symLenGain * phaseError);
                if (dynSampPerSym > SampPerSymMax) { dynSampPerSym = SampPerSymMax; }
                else if (dynSampPerSym < SampPerSymMin) { dynSampPerSym = SampPerSymMin; }

                // Adjust the symbol phase according to the phase error approximation
                // It will now contain the phase delta needed to jump to the next symbol
                // Rounded step will contain the rounded number of symbols
                symPhase = symPhase + dynSampPerSym + (_symPhaseGain * phaseError);
                roundedStep = floor(symPhase);

                // Step to where the next symbol should be
                i += roundedStep;

                // Now that we've stepped to the next symbol, keep only the offset inside the symbol
                symPhase -= roundedStep;
            }
            
            _in->flush();
            if (!out.swap(outCount)) { return -1; }
            return count;
        }

        stream<float> out;

    private:
        int count;

        // Configuration
        float _sampPerSym = 1.0f;
        float _symPhaseGain = 1.0f;
        float _symLenGain = 0.001f;
        float _symRateDelta = 0.005;

        // Precalculated values
        float SampPerSymMin = _sampPerSym + (_sampPerSym * _symRateDelta);
        float SampPerSymMax = _sampPerSym + (_sampPerSym * _symRateDelta);

        // Runtime adjusted
        float dynSampPerSym = _sampPerSym;
        float symPhase = 0.5f;
        float lastOutput = 0.0f;
        float lastInput = 0.0f;
        

        stream<float>* _in;

    };
}