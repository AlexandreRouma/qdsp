#pragma once
#include <dsp/block.h>

namespace dsp {
    template <class T>
    class Splitter : public generic_block<Splitter<T>> {
    public:
        Splitter() {}

        Splitter(stream<T>* in) { init(in); }

        ~Splitter() { generic_block::stop(); }

        void init(stream<T>* in) {
            _in = in;
            generic_block::registerInput(_in);
        }

        void setInput(stream<T>* in) {
            std::lock_guard<std::mutex> lck(ctrlMtx);
            generic_block::tempStop();
            generic_block::unregisterInput(_in);
            _in = in;
            generic_block::registerInput(_in);
            generic_block::tempStart();
        }

        void bindStream(stream<T>* stream) {
            std::lock_guard<std::mutex> lck(ctrlMtx);
            generic_block::tempStop();
            out.push_back(stream);
            generic_block::registerOutput(stream);
            generic_block::tempStart();
        }

        void unbindStream(stream<T>* stream) {
            std::lock_guard<std::mutex> lck(ctrlMtx);
            generic_block::tempStop();
            generic_block::unregisterOutput(stream);
            out.erase(std::remove(out.begin(), out.end(), stream), out.end());
            generic_block::tempStart();
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