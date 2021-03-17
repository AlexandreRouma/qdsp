#pragma once
#include <dsp/block.h>
#include <inttypes.h>

namespace dsp {
    struct FalconFrameHeader {
        uint32_t counter;
        uint16_t packet;
    };

    class FalconPacketSync : public generic_block<FalconPacketSync> {
    public:
        FalconPacketSync() {}

        FalconPacketSync(stream<uint8_t>* in) { init(in); }

        ~FalconPacketSync() {
            generic_block<FalconPacketSync>::stop();
        }

        void init(stream<uint8_t>* in) {
            _in = in;
            
            generic_block<FalconPacketSync>::registerInput(_in);
            generic_block<FalconPacketSync>::registerOutput(&out);
        }

        int run() {
            count = _in->read();
            if (count < 0) { return -1; }

            // Parse frame header
            FalconFrameHeader header;
            header.packet = (_in->readBuf[3] | ((_in->readBuf[2] & 0b111) << 8));
            header.counter = ((_in->readBuf[2] >> 3) | (_in->readBuf[1] << 5) | ((_in->readBuf[0] & 0b111111) << 13));

            // Pointer to the data aera of the frame
            uint8_t* data = _in->readBuf + 4;
            int dataLen = count - 4;

            //printf("[%d] %d\n", header.counter, header.packet);

            // If a frame was missed, cancel reading the current packet
            if (lastCounter + 1 != header.counter) {
                packetRead = -1;
            }
            lastCounter = header.counter;
            lastWasVideo = false;

            // If frame is just a continuation of a single packet, save it
            // If we're not currently reading a packet
            if (header.packet == 2047 && packetRead >= 0) {
                memcpy(packet, data, dataLen);
                packetRead += dataLen;
                _in->flush();
                return count; 
            }
            else if (header.packet == 2047) {
                _in->flush();
                return count; 
            }

            // Finish reading the last package and send it
            if (packetRead >= 0) {
                memcpy(packet + packetRead, data, header.packet);
                memcpy(out.writeBuf, packet, packetRead + header.packet);
                out.swap(packetRead + header.packet);
                packetRead = -1;
            }

            // Iterate through every packet of the frame
            int iteration = 0;
            for (int i = header.packet; i < dataLen;) {

                // First, check if we can read the header. If not, save and wait for next frame
                if (dataLen - i < 4) {
                    printf("packet header didn't fit\n");
                    packetRead = dataLen - i;
                    memcpy(packet, &data[i], packetRead);
                    break;
                }

                // Extract the packet length from the packet header (add two for the CRC, and 4 for the header)
                uint16_t length = (((data[i] & 0b1111) << 8) | data[i + 1]) + 2;

                // Some packet have a zero length, just get rid of the frame
                if (length <= 2) {
                    packetRead = -1;
                    break;
                }

                // 4,854,340

                uint64_t pktId =    ((uint64_t)data[i + 2] << 56) | ((uint64_t)data[i + 3] << 48) | ((uint64_t)data[i + 4] << 40) | ((uint64_t)data[i + 5] << 32)
                                |   ((uint64_t)data[i + 6] << 24) | ((uint64_t)data[i + 7] << 16) | ((uint64_t)data[i + 8] << 8) | data[i + 9];

                // If the packet doesn't fit the frame, save and go to next frame
                if (dataLen - i < length) {
                    packetRead = dataLen - i;
                    memcpy(packet, &data[i], packetRead);
                    break;
                }

                // Here, the package fits fully, read it and jump to the next
                memcpy(out.writeBuf, &data[i], length);
                //out.swap(length);
                i += length;

                iteration++;

            }

            _in->flush();
            return count;
        }

        stream<uint8_t> out;

    private:
        int count;
        uint32_t lastCounter = 0;

        bool lastWasVideo = false;

        int packetRead = -1;
        uint8_t packet[0x4008];
        
        stream<uint8_t>* _in;

    };
}