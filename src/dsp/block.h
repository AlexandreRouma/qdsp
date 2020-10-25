#pragma once
#include <stdio.h>
#include <dsp/stream.h>
#include <dsp/types.h>
#include <thread>
#include <vector>
#include <algorithm>

#define FL_M_PI                3.1415926535f

namespace dsp {
    
    template <class BLOCK>
    class generic_block {
    public:
        virtual void init() {}

        virtual void start() {
            std::lock_guard<std::mutex> lck(ctrlMtx);
            if (running) {
                return;
            }
            doStart();
        }

        virtual void stop() {
            std::lock_guard<std::mutex> lck(ctrlMtx);
            if (!running) {
                return;
            }
            doStop();
        }

        virtual int calcOutSize(int inSize) { return -1; }

        virtual int run() = 0;
        
        friend BLOCK;

    private:
        void workerLoop() { 
            while (run() >= 0);
        }

        void aquire() {
            ctrlMtx.lock();
        }

        void release() {
            ctrlMtx.unlock();
        }

        void registerInput(untyped_steam* inStream) {
            inputs.push_back(inStream);
        }

        void unregisterInput(untyped_steam* inStream) {
            inputs.erase(std::remove(inputs.begin(), inputs.end(), inStream), inputs.end());
        }

        void registerOutput(untyped_steam* outStream) {
            outputs.push_back(outStream);
        }

        void unregisterOutput(untyped_steam* outStream) {
            outputs.erase(std::remove(outputs.begin(), outputs.end(), outStream), outputs.end());
        }

        void doStart() {
            running = true;
            workerThread = std::thread(&generic_block::workerLoop, this);
        }

        void doStop() {
            for (auto const& in : inputs) {
                in->stopReader();
            }
            for (auto const& out : outputs) {
                out->stopWriter();
            }
            workerThread.join();
            for (auto const& in : inputs) {
                in->clearReadStop();
            }
            for (auto const& out : outputs) {
                out->clearWriteStop();
            }
        }

        void tempStart() {
            if (tempStopped) {
                doStart();
                tempStopped = false;
            }
        }

        void tempStop() {
            if (running) {
                doStop();
                tempStopped = true;
            }
        }

        std::vector<untyped_steam*> inputs;
        std::vector<untyped_steam*> outputs;

        bool running = false;
        bool tempStopped = false;

        std::thread workerThread;

    protected:
        std::mutex ctrlMtx;

    };
}