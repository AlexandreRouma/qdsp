#pragma once
#include <dsp/block.h>

namespace dsp {
    template <class T>
    class Splitter : public generic_block<Splitter<T>> {
    public:
        Splitter() {}

        Splitter(stream<T>* in) { init(in); }

        ~Splitter() { stop(); }

        void init(stream<T>* in) {
            _in = in;
            registerInput(_in);
        }

        void setInput(stream<T>* in) {
            std::lock_guard<std::mutex> lck(ctrlMtx);
            tempStop();
            unregisterInput(_in);
            _in = in;
            registerInput(_in);
            tempStart();
        }

        void bindStream(stream<T>* stream) {
            std::lock_guard<std::mutex> lck(ctrlMtx);
            tempStop();
            out.push_back(stream);
            registerOutput(stream);
            tempStart();
        }

        void unbindStream(stream<T>* stream) {
            std::lock_guard<std::mutex> lck(ctrlMtx);
            tempStop();
            unregisterOutput(stream);
            out.erase(std::remove(out.begin(), out.end(), stream), out.end());
            tempStart();
        }

    private:
        int run() {
            // TODO: If too slow, buffering might be necessary
            int count = _in->read();
            if (count < 0) { return -1; }
            for (const auto& stream : out) {
                stream->aquire();
                memcpy(stream->data, _in->data, count * sizeof(T));
                stream->write(count);
            }
            _in->flush();
            return count;
        }

        stream<T>* _in;
        std::vector<stream<T>*> out;

    };
}