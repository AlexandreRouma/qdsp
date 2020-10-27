#pragma once
#include <mutex>
#include <condition_variable>
#include <volk/volk.h>

// 1M Values Buffer
#define STREAM_BUFFER_SIZE  1000000

namespace dsp {
    class untyped_steam {
    public:
        virtual int write(int size) { return -1; }
        virtual int read() { return -1; }
        virtual void flush() {}
        virtual void stopReader() {}
        virtual void clearReadStop() {}
        virtual void stopWriter() {}
        virtual void clearWriteStop() {}
    };

    template <class T>
    class stream : public untyped_steam {
    public:
        stream() {
            data = (T*)volk_malloc(STREAM_BUFFER_SIZE * sizeof(T), volk_get_alignment());
        }
        
        //Open stream for writing
        int aquire() {
            waitReady();
            if (writerStop) {
                return -1;
            }
            return 0;
        }
        
        //Writing {size} values done, close stream for writing
        int write(int size) {
            std::lock_guard<std::mutex> lck(sigMtx);
            contentSize = size;
            dataReady = true;
            cv.notify_all();
            return 0;
        }
        
        //Try to read data available in buffer
        int read() {
            waitData();
            if (readerStop) {
                return -1;
            }
            return contentSize;
        }
        
        //Reading done
        void flush() {
            std::lock_guard<std::mutex> lck(sigMtx);
            dataReady = false;
            cv.notify_all();
        }

        void stopReader() {
            std::lock_guard<std::mutex> lck(sigMtx);
            readerStop = true;
            cv.notify_all();
        }

        void clearReadStop() {
            readerStop = false;
        }

        void stopWriter() {
            std::lock_guard<std::mutex> lck(sigMtx);
            writerStop = true;
            cv.notify_all();
        }

        void clearWriteStop() {
            writerStop = false;
        }

        T* data;

    private:
        void waitReady() {
            std::unique_lock<std::mutex> lck(sigMtx);
            cv.wait(lck, [this]{ return !dataReady || writerStop; });
        }

        void waitData() {
            std::unique_lock<std::mutex> lck(sigMtx);
            cv.wait(lck, [this]{ return dataReady || readerStop; });
        }

        std::mutex sigMtx;
        std::condition_variable cv;
        bool dataReady = false;

        bool readerStop = false;
        bool writerStop = false;

        int contentSize = 0;
    };
}
