#pragma once
#include <dsp/block.h>
#include <volk/volk.h>
#include <SoapySDR/Device.hpp>
#include <SoapySDR/Types.hpp>
#include <SoapySDR/Formats.hpp>
#include <map>

//Beta SoapySDR Source block

namespace dsp {
    class SoapysdrSource : public source_block<SoapysdrSource> {
    public:
        
        SoapysdrSource() { init(); }

        ~SoapysdrSource() { 
            source_block<SoapysdrSource>::stop(); 
            releaseDevice(); //Release device, channel and stream on destruction
        }
        
        //Initialize block
        void init() {
            updateDevicelist();
            device_hdl = NULL;
            device_str = NULL;
            currentDeviceId = channel = -1;
            frequency = frequencyCorrection = sampleRate = bandwidth = -1; //Reset all pointers and variables
            source_block<SoapysdrSource>::registerOutput(&out);
        }
        
        //Update device list
        void updateDevicelist() {
            devices_list = SoapySDR::Device::enumerate();
        }
        
        //Get device list
        SoapySDR::KwargsList getDeviceList() {
            return devices_list;
        }
        
        //Select device by index(return 0-ok, -1 - failed)
        int selectDevice(int devIndex) {
            if(currentDeviceId != -1)
                return -1;
            
            std::lock_guard<std::mutex> lck(dev_mtx);
            SoapySDR::Kwargs devArgs = devices_list[devIndex];
            device_hdl = SoapySDR::Device::make(devArgs);
            if(device_hdl == NULL) {
                fprintf(stderr, "SoapySDR::Device::make() failed\n");
                return -1;
            }
            currentDeviceId = devIndex;
            int numchannels = device_hdl->getNumChannels(SOAPY_SDR_RX); //Load channels id's and descriptions
            for(int i = 0; i < numchannels; i++) {
                channels.insert(std::pair<int, SoapySDR::Kwargs> (i, device_hdl->getChannelInfo(SOAPY_SDR_RX, i)));
            }
            return 0;
        }
        
        //Get available channels
        std::map <int, SoapySDR::Kwargs> getAvailableChannels() {
            return channels;
        }
        
        //Select channel by index(return 0-ok, -1 - failed)
        int selectChannel(int channelIndex) {
            if(currentDeviceId == -1)
                return -1; //If device not selected
            if(channels.count(channelIndex) < 1 || channel != -1)
                return -1; //If channel not present
            std::lock_guard<std::mutex> lck(dev_mtx);
            channel = channelIndex;
                
            if(device_hdl->hasGainMode(SOAPY_SDR_RX, channel))
                autoGain = device_hdl->getGainMode(SOAPY_SDR_RX, channel); //Read autogain value
                
            if(device_hdl->hasFrequencyCorrection(SOAPY_SDR_RX, channel))
                frequencyCorrection = device_hdl->getFrequencyCorrection(SOAPY_SDR_RX, channel); //Read frequency correction value
                
            sampleRate = device_hdl->getSampleRate(SOAPY_SDR_RX, channel); //Read sample rate and available variants
            sampleRateRanges = device_hdl->getSampleRateRange(SOAPY_SDR_RX, channel);
                
            bandwidth = device_hdl->getBandwidth(SOAPY_SDR_RX, channel); //Read bandwidth and available variants
            bandwidthRanges = device_hdl->getBandwidthRange(SOAPY_SDR_RX, channel);
                
            antennas = device_hdl->listAntennas(SOAPY_SDR_RX, channel); //Read available antennas and current
            antenna = device_hdl->getAntenna(SOAPY_SDR_RX, channel);
                
            std::vector <std::string> gainsNames = device_hdl->listGains(SOAPY_SDR_RX, channel); //Load current gains and its ranges
            for(int i=0; i < gainsNames.size(); i++) {
                double currentGain = device_hdl->getGain(SOAPY_SDR_RX, channel, gainsNames[i]);
                SoapySDR::Range gainRange = device_hdl->getGainRange(SOAPY_SDR_RX, channel, gainsNames[i]);
                gains.insert(std::pair <std::string, double> (gainsNames[i], currentGain));
                gains_ranges.insert(std::pair <std::string, SoapySDR::Range> (gainsNames[i], gainRange));
            }
                
                
            frequency = device_hdl->getFrequency(SOAPY_SDR_RX, channel); //Read frequency and range
            frequency_ranges = device_hdl->getFrequencyRange(SOAPY_SDR_RX, channel);
            
            return 0;
        }
        
        //Receiver has auto gain mode?
        bool hasAutoGain() {
            if(channel != -1)
                return device_hdl->hasGainMode(SOAPY_SDR_RX, channel);
            else
                return false;
        }
        
        //Try to set autogain mode(return 0-ok, -1 - fail)
        int setAutoGain(bool gainMode) {
            if(channel == -1 || !hasAutoGain())
                return -1;
            std::lock_guard<std::mutex> lck(dev_mtx);
            autoGain = gainMode;
            device_hdl->setGainMode(SOAPY_SDR_RX, channel, autoGain);
            return 0;
        }
        
        //Return current autogain mode
        bool getAutoGain() {
            return autoGain;
        }
        
        //Receiver has frequency correction setting?
        bool hasFrequencyCorrection() {
            if(channel != -1) 
                return device_hdl->hasFrequencyCorrection(SOAPY_SDR_RX, channel);
            else
                return false;
        }
        
        //Try to set frequency correction(return 0-ok, -1 - fail)
        int setFrequencyCorrection(double freqCorrPpm) {
            if(channel == -1 || !hasFrequencyCorrection())
                return -1;
            std::lock_guard<std::mutex> lck(dev_mtx);
            frequencyCorrection = freqCorrPpm;
            device_hdl->setFrequencyCorrection(SOAPY_SDR_RX, channel, frequencyCorrection);
            return 0;
        }
        
        //Return current frequency correction
        double getFrequencyCorrection() {
            return frequencyCorrection;
        }
        
        //Return all available sample rates
        SoapySDR::RangeList getSampleRateRanges() {
            return sampleRateRanges;
        }
        
        //Try to set sample rate(return 0-ok, -1 - fail, -2 - not in range)
        int setSampleRate(double sr) {
            if(channel == -1)
                return -1;
            std::lock_guard<std::mutex> ctllck(source_block<SoapysdrSource>::ctrlMtx);
            std::lock_guard<std::mutex> lck(dev_mtx);
            source_block<SoapysdrSource>::tempStop();
            if(!inRangeList(sampleRateRanges, sr))
                return -2;
            sampleRate = sr;
            device_hdl->setSampleRate(SOAPY_SDR_RX, channel, sampleRate);
            source_block<SoapysdrSource>::tempStart();
            return 0;
        }
        
        //Return current sample rate
        double getSampleRate() {
            return sampleRate;
        }
        
        //Return all available bandwidths
        SoapySDR::RangeList getBandwidthRanges() {
            return bandwidthRanges;
        }
        
        //Try to set bandwidth(return 0-ok, -1 - fail, -2 - not in range)
        int setBandwidth(double bw) {
            if(channel == -1)
                return -1;
            std::lock_guard<std::mutex> lck(dev_mtx);
            if(!inRangeList(bandwidthRanges, bw))
                return -2;
            bandwidth = bw;
            device_hdl->setBandwidth(SOAPY_SDR_RX, channel, bandwidth);
            return 0;
        }
        
        //Return current bandwidth
        double getBandwidth() {
            return bandwidth;
        }
        
        //Return all available antennas
        std::vector <std::string> getAntennas() {
            return antennas;
        }
        
        //Try to set antenna(return 0-ok, -1 - fail)
        int setAntenna(std::string name) {
            if(channel == -1 || std::find(antennas.begin(), antennas.end(), name) == antennas.end())
                return -1;
            std::lock_guard<std::mutex> lck(dev_mtx);
            antenna = name;
            device_hdl->setAntenna(SOAPY_SDR_RX, channel, antenna);
            return 0;
        }
        
        //Return current antenna
        std::string getAntenna() {
            return antenna;
        }
        
        //Return available gains and its ranges in map
        std::map <std::string, SoapySDR::Range> getGainsRanges() {
            return gains_ranges;
        }
        
        //Try to set specified gain(return 0-ok, -1 - fail, -2 - not in range)
        int setGain(std::string name, double g) {
            if(channel == -1 || gains.count(name) < 1)
                return -1;
            std::lock_guard<std::mutex> lck(dev_mtx);
            SoapySDR::Range r = gains_ranges.at(name);
            if(
                g < r.minimum() || 
                g > r.maximum() || 
                (
                    r.step() != 0 && //Check if step provided
                    ((int)(g*100) % (int)(r.step()*100) != 0) //Crunch to make it process float vals(up to one hundredth)
                ))
                return -2;
            gains.at(name) = g;
            device_hdl->setGain(SOAPY_SDR_RX, channel, name, g);
            return 0;
        }
        
        //Get specified gain
        double getGain(std::string name) {
            if(channel != -1 && gains.count(name) > 0) {
                return gains.at(name);
            } else
                return -1;
        }
        
        //Return frequency ranges
        SoapySDR::RangeList getFrequencyRanges() {
            return frequency_ranges;
        }
        
        //Try to set frequency(return 0-ok, -1 - fail, -2 - not in range)
        int setFrequency(double freq) {
            if(channel == -1)
                return -1;
            std::lock_guard<std::mutex> lck(dev_mtx);
            if(!inRangeList(frequency_ranges, freq))
                return -2;
            frequency = freq;
            device_hdl->setFrequency(SOAPY_SDR_RX, channel, freq);
            return 0;
        }    
        
        //Return current frequency
        double getFrequency() {
            if(channel != -1) {
                return frequency;
            } else
                return -1;
        }
        
        //Initialize and start data stream(return 0-ok, -1 - fail)
        int initStream() {
            if(device_available)
                return -1;
            std::lock_guard<std::mutex> lck(dev_mtx);
            std::unique_lock<std::mutex> locker(devavail_mtx); //Lock device available mutex
            device_str = device_hdl->setupStream(SOAPY_SDR_RX, SOAPY_SDR_CF32);
            if(device_str == NULL) {
                fprintf(stderr, "SoapySDR::Device::setupStream() failed\n");
                return -1;
            }
            int ret;
            if((ret = device_hdl->activateStream(device_str, 0, 0, 0)) != 0) {
                fprintf(stderr, "SoapySDR::Device::activateStream() failed: %d\n", ret);
                return -1;
            }
            device_available = true; //Set device available
            locker.unlock(); //Unlock this, so worker thread don't hang
            device_available_cv.notify_all(); //Notify worker thread, that device is now available
            return 0;
        }
        
        //Stop data stream
        void stopStream() {
            if(!device_available)
                return;
            std::lock_guard<std::mutex> lck(dev_mtx);
            device_hdl->deactivateStream(device_str, 0, 0);
            device_hdl->closeStream(device_str);
            device_str = NULL;
            device_available = false;
        }
        
        //Clear channel selection
        void deselectChannel() {
            if(device_available)
                stopStream();
            if(channel == -1)
                return;
            std::lock_guard<std::mutex> lck(dev_mtx);
            autoGain = false;
            frequencyCorrection = -1;
            sampleRate = -1;
            sampleRateRanges.clear();
            bandwidth = -1;
            bandwidthRanges.clear();
            antennas.clear();
            antenna = "";
            gains.clear();
            gains_ranges.clear();
            frequency = -1;
            frequency_ranges.clear();
            channel = -1;
        }
        
        //Disconnect from device
        void releaseDevice() {
            if(channel != -1) 
                deselectChannel();
            if(currentDeviceId == -1)
                return;
            SoapySDR::Device::unmake(device_hdl);
            device_hdl = NULL;
            currentDeviceId = -1;
        }
        
        //Check if value in range of range list
        bool inRangeList(SoapySDR::RangeList rl, double val) {
            bool inRange = false;
            for(int i=0; i < rl.size(); i++) {
                SoapySDR::Range r = rl[i];
                if(val >= r.minimum() && val <= r.maximum()) {
                    if(r.step() != 0 && ((int)(val*100) % (int)(r.step()*100) != 0)) //Crunch to make it process float vals
                        continue;
                    inRange = true;
                }
            }
            return inRange;
        }

        int run() {
            while(!device_available) {
                std::unique_lock<std::mutex> locker(devavail_mtx);
                device_available_cv.wait_for(locker, std::chrono::milliseconds(3000), [this]{return this->device_available;}); //If stream started before stream, wait for stream to start
            }
            std::lock_guard<std::mutex> lck(dev_mtx);
            if(!device_available)
                return 0;
            int ret = out.aquire();
            if(ret != 0)
                return ret; //If stream is closed
            int nitems = sampleRate/200; //Samples to read from device at one iteration
            int flags;
            long long time_ns;
            do {
                ret = device_hdl->readStream(device_str, (void**)&out.data, nitems, flags, time_ns, 1e6);
            } while(ret == -1); // While new data is not avilable, try reading
            if(ret > 0)
                nitems = ret;
            else {
                out.write(0);
                if(ret != -4) //Overflow is ok
                    return ret; //If error occured
                return 0;
            }
            out.write(nitems);
            return nitems;
        }

        stream<complex_t> out;

    private:
        SoapySDR::Device* device_hdl;
        SoapySDR::Stream* device_str;
        SoapySDR::KwargsList devices_list;
        int currentDeviceId;
        
        std::map <int, SoapySDR::Kwargs> channels;
        int channel;
        
        bool autoGain = false;
        
        double frequencyCorrection;
        
        double sampleRate;
        SoapySDR::RangeList sampleRateRanges;
        
        double bandwidth;
        SoapySDR::RangeList bandwidthRanges;
        
        std::vector<std::string> antennas;
        std::string antenna;
        
        std::map <std::string, double> gains;
        std::map <std::string, SoapySDR::Range> gains_ranges;
        
        double frequency;
        SoapySDR::RangeList frequency_ranges;
        
        std::mutex dev_mtx;
        std::mutex devavail_mtx;
        std::condition_variable device_available_cv;
        bool device_available = false;
    };
}
