#pragma once
#include <stdio.h>
#include <dsp/stream.h>
#include <dsp/types.h>
#include <thread>
#include <vector>

namespace dsp {
    
    template <class BLOCK>
    class generic_block {
    public:
        virtual void init() {}

        virtual void start() {
            aquire();
            if (running) {
                return;
            }
            running = true;
            workerThread = std::thread(&generic_block::workerLoop, this);
            release();
        }

        virtual void stop() {
            aquire();
            if (!running) {
                release();
                return;
            }
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
            release();
        }

        virtual int calcOutSize(int inSize) { return -1; }

        virtual int run() {
            printf("UH OH: BASE WORKER RAN!!!\n");
            return -1;
        }
        
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

        void registerInput(untyped_steam* inSteam) {
            inputs.push_back(inSteam);
        }

        void unregisterInput(untyped_steam* inSteam) {
            
        }

        void registerOutput(untyped_steam* outStream) {
            outputs.push_back(outStream);
        }

        void unregisterOutput(untyped_steam* outStream) {
            
        }

        std::vector<untyped_steam*> inputs;
        std::vector<untyped_steam*> outputs;

        std::mutex ctrlMtx;

        bool running = false;

        std::thread workerThread;

    };
}