#include "AudioProcessor.h"

#define STB_VORBIS_HEADER_ONLY
#include "../external/miniaudio/extras/stb_vorbis.c"    // Enables Vorbis decoding.

// this should only exist in one translation unit or it will cause a link error for some reason
#define MINIAUDIO_IMPLEMENTATION
#include "../external/miniaudio/miniaudio.h"
//wtf
// The stb_vorbis implementation must come after the implementation of miniaudio.
//#undef STB_VORBIS_HEADER_ONLY
//#include "../external/miniaudio/extras/stb_vorbis.c"

#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>


using namespace AVis;

AudioProcessor::AudioProcessor() {
	//create the decayThread
    ma_fence_init(decayThreadData.fence);
    decayThreadData.processor = this;
    decayThreadData.end = false;
    ma_thread_entry_proc entryProc = (ma_thread_entry_proc) &decayEnergyPasses;
    ma_thread_create(&decayThread, ma_thread_priority_idle, 32, entryProc, &decayThreadData, nullptr);
}
AudioProcessor::~AudioProcessor() {
    //free all instances and devices
    destroyAllDevices();
    destroyAllEnergyMappers();

    //destro the decaythread
    decayThreadData.end = true;
    ma_thread_wait(&decayThread);

    ma_fence_uninit(decayThreadData.fence);
}

void AudioProcessor::setTrackEnergy(size_t energyInd, size_t soundInd, float minFreq, float maxFreq,
            float minAmp, float maxAmp, float decayRate) {
    if (devices.size() <= soundInd || devices[soundInd] == nullptr) {
        //no sound to process
        std::cerr << "No sound loaded at index: " << soundInd << std::endl;
        return;
    }
    
    if (energies.size() <= energyInd) {
        energies.resize(energyInd + 4);
        decayRates.resize(energyInd + 4);
        energyMappers.resize(energyInd + 4);
    }
    if (energyMappers[energyInd] == nullptr) {
        //create and initialise an energy mapper if none exists here
        DeviceData *ddata = (DeviceData*)(devices[soundInd]->pUserData);
        ma_decoder* decoder = ddata->decoder;

        EnergyMapper* emap = new EnergyMapper(soundInd, minFreq, maxFreq, minAmp, maxAmp, 
                decoder->outputFormat, decoder->outputChannels, decoder->outputSampleRate);
        energyMappers[energyInd] = emap;
    }
    else {
        energyMappers[energyInd]->reinitialise(minFreq, maxFreq, minAmp, maxAmp);
    }

    decayRates[energyInd] = decayRate;
}

std::vector<float> AudioProcessor::getEnergies() {
    return energies;
}

void AudioProcessor::playAll(bool restart) {
    for (size_t i = 0; i < devices.size(); i++) {
        ma_device *device = devices[i];
        if (device != nullptr) {
            if (restart) {
                ma_decoder* decoder = ((DeviceData*)device->pUserData)->decoder;
                ma_decoder_seek_to_pcm_frame(decoder, 0);
            }
            ma_device_start(device);
        }
    }
    playing = true;
}

void AudioProcessor::stopAll(bool restart) {
    for (size_t i = 0; i < devices.size(); i++) {
        ma_device* device = devices[i];
        if (device != nullptr) {
            if (restart) {
                //if they press a STOP button as opposed to a pause button
                ma_decoder* decoder = ((DeviceData*)device->pUserData)->decoder;
                ma_decoder_seek_to_pcm_frame(decoder, 0);
            }
            ma_device_stop(device);
        }
    }
    playing = false;
}

bool AudioProcessor::isPlaying() {
    return playing;
}

size_t AudioProcessor::addSoundFromFile(const char *filePath) {
    size_t theInd;

    if (availableIndices.empty()) {
        theInd = devices.size();
        devices.resize(devices.size() + 8);
    }
    else {
        theInd = availableIndices.back();
        availableIndices.pop_back();
    }

    ma_device *device = new ma_device;
    if (createDeviceFromFile(device, filePath, theInd) != Result::SUCCESS) {
        std::cerr << "Could not load file at " << filePath << std::endl;
        return UINT16_MAX;
    }

    devices[theInd] = device;

    //add the new device in the local vector to be accessed later
    return theInd;
}

AudioProcessor::Result AudioProcessor::changeAudioFile(size_t soundId, const char *fileName) {

    ma_decoder* decoder = new ma_decoder;
    if (ma_decoder_init_file(fileName, nullptr, decoder) != MA_SUCCESS) {
        delete decoder;
        return Result::LOAD_ERROR;
    }
    //replace the decoder
    DeviceData* ddata = (DeviceData*)devices[soundId]->pUserData;
    delete ddata->decoder;
    ddata->decoder = decoder;
    
    return Result::SUCCESS;
}


void AudioProcessor::EnergyMapper::reinitialise(float minF, float maxF, float minA, float maxA) {
    ma_lpf1_config lowpassConfig = ma_lpf1_config_init(ma_format_f32, channels, sampleRate, maxF);
    ma_hpf1_config highpassConfig = ma_hpf1_config_init(ma_format_f32, channels, sampleRate, minF);
    
    ma_lpf1_reinit(&lowpassConfig, lowpass);
    ma_hpf1_reinit(&highpassConfig, highpass);
}

AudioProcessor::EnergyMapper::EnergyMapper(size_t id, float minF, float maxF, float minA, float maxA, ma_format form, ma_uint32 chan, ma_uint32 samp)
        : soundId(id), minFreq(minF), maxFreq(maxF), minAmp(minA), maxAmp(maxA), format(form), channels(chan), sampleRate(samp) {
    // while the mapping can happen to sounds of all formats, the filtering will produce a float
    ma_lpf1_config lowpassConfig = ma_lpf1_config_init(ma_format_f32, chan, samp, maxF);
    ma_hpf1_config highpassConfig = ma_hpf1_config_init(ma_format_f32, chan, samp, minF);
    
    lowpass = new ma_lpf1;
    highpass = new ma_hpf1;

    ma_lpf1_init(&lowpassConfig, nullptr, lowpass);
    ma_hpf1_init(&highpassConfig, nullptr, highpass);
}

AudioProcessor::EnergyMapper::~EnergyMapper() {
    ma_lpf1_uninit(lowpass, nullptr);
    ma_hpf1_uninit(highpass, nullptr);
}

AudioProcessor::Result AudioProcessor::createDeviceFromFile(ma_device* device, const char* fileName, size_t id) {
    //create a decoder that will read the filedata
    ma_decoder* decoder = new ma_decoder;
    if (ma_decoder_init_file(fileName, nullptr, decoder) != MA_SUCCESS) {
        delete decoder;
        return Result::LOAD_ERROR;
    }

    //create pUserData of the device
    DeviceData* ddata = new DeviceData;
    ddata->id = id;
    ddata->decoder = decoder;
    ddata->processor = this;

    //create device
    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = decoder->outputFormat;
    deviceConfig.playback.channels = decoder->outputChannels;
    deviceConfig.sampleRate = decoder->outputSampleRate;
    deviceConfig.dataCallback = dataCallback;
    deviceConfig.pUserData = ddata;

    //failed abort
    if (ma_device_init(nullptr, &deviceConfig, device) != MA_SUCCESS) {
        ma_decoder_uninit(decoder);
        delete ddata;
        delete decoder;
        return Result::LOAD_ERROR;
    }
    const float volume = 0.5;
    ma_device_set_master_volume(device, volume);
    return Result::SUCCESS;
}

void AudioProcessor::dataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    DeviceData* ddata = (DeviceData*)pDevice->pUserData;
    ma_decoder* decoder = ddata->decoder;
    if (decoder == NULL) {
        return;
    }
    ma_uint64 framesRead;
    ma_decoder_read_pcm_frames(decoder, pOutput, frameCount, &framesRead);
    //set all the relevant energys
    AudioProcessor* processor = ddata->processor;
    auto mappers = processor->energyMappers;
    for (size_t i = 0; i < mappers.size(); i++) {
        EnergyMapper* emap = mappers[i];
        if (emap != nullptr && emap->soundId == ddata->id) {
            float newEnergy = calcEnergy(pOutput, framesRead, emap);
            //increase the energy of that peak up till the new amount
            processor->energies[i] = max(newEnergy, processor->energies[i]);
        }
    }

    (void)pInput;
}

//helper function finds the max value in an array of floats. I could use templates but there is only one usage of it
static inline float maxAmp(float* arr, size_t size) {
    float cand = 0;
    for (size_t i = 0; i < size; i++) {
        float value = arr[i] >= 0.0 ? arr[i] : -arr[i];
        cand = value > cand ? value : cand;
    }
    return cand;
}

float AudioProcessor::calcEnergy(const void* frames, ma_uint64 nFrames, EnergyMapper *mapper) {
    
    //copy input frames converted to 32 bit floats and marks new data for freeing, if neccissary
    void* convertedFrames;
    convertedFrames = malloc(sizeof(float) * nFrames * mapper->channels);
    ma_convert_pcm_frames_format(convertedFrames, ma_format_f32, frames, mapper->format, nFrames, mapper->channels, ma_dither_mode_none);

    ma_lpf1_process_pcm_frames(mapper->lowpass, convertedFrames, convertedFrames, nFrames);
    ma_hpf1_process_pcm_frames(mapper->highpass, convertedFrames, convertedFrames, nFrames);

    float* framesAsFloat = (float*)convertedFrames;
    //we're using max, not average of the last signals
    float energy = maxAmp(framesAsFloat, nFrames);

    //clamp and scale energy to min and max amplitudes. Inverse lerp from min and max amps
    energy = std::clamp((energy - mapper->minAmp) / (mapper->maxAmp - mapper->minAmp), 0.0f, 1.0f);


    free(framesAsFloat);
    return energy;
}

void AudioProcessor::decayEnergyPasses(void* decayThreadData) {
    static const float frameFreq = 1.0f / 60.0f; // seconds per frame
    static const float decayPeriodMilli = 1000.0f * frameFreq; // time in milliseconds between each tick where enerygies are lowered 

    DecayThreadData* decayData = (DecayThreadData*)decayThreadData;
    AudioProcessor* processor = decayData->processor;
    ma_fence* fence = decayData->fence;
    while (decayData->end == false) {
        //lock up the processor for the duration of the pass so devices dont update it while decays are happening
        //ma_fence_acquire(fence);
        for (size_t i = 0; i < processor->energies.size(); i++) {
            float decayAmount = frameFreq * processor->decayRates[i];
            processor->energies[i] = max(processor->energies[i] - decayAmount, 0.0f);

        }
        //ma_fence_release(fence);
        ma_sleep(decayPeriodMilli);
    }
    
}

void AudioProcessor::destroyDevice(ma_device* device) {
    DeviceData *ddata = (DeviceData*)device->pUserData;
    ma_decoder *decoder = ddata->decoder;
    ma_device_uninit(device);
    ma_decoder_uninit(decoder);
    delete decoder;
    delete ddata;
    
}


void AudioProcessor::destroyAllDevices() {
    for (ma_device *device : devices) {
        if (device != nullptr) {
            destroyDevice(device);
        }
    }
}

void AudioProcessor::destroyAllEnergyMappers() {
    for (EnergyMapper* mapper : energyMappers) {
        if (mapper != nullptr) {
            delete mapper;
        }
    }
}