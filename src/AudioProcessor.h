#ifndef AVIS_AUDIO_PROCESSOR
#define AVIS_AUDIO_PROCESSOR


#include "../external/miniaudio/miniaudio.h"
#include <vector>

namespace AVis {
class AudioProcessor {
	// Object responsible for audio playback and converting signals into array of energies to be used by the visualiser

public:
	//flags to handle errors so app doesnt just crash if we can't load a file
	enum Result { SUCCESS, LOAD_ERROR };
	
	AudioProcessor();
	~AudioProcessor();

	void playAll(bool restart = false);

	void stopAll(bool restart = false);
	// playing is read only
	bool isPlaying();


	// will cause AudioProcessor to produce a value in getEnergy for that frequency range and that sound
	void setTrackEnergy(size_t energyInd, size_t soundInd, float minFreq, float maxFreq, float minAmp, float maxAmp, float decayRate = 1.4f);

	//adds a sound and returns an id with which it can be accessed. -1 means it failed to load
	size_t addSoundFromFile(const char* fileName);

	//give the 
	Result changeAudioFile(size_t soundId, const char* filename);
	
	//produces the energies of visualiser peaks based on current sound buffers. values range [0, 1]
	std::vector<float> getEnergies();

private:

	//A device is plaaying/recording sound
	bool playing = false;

	// devices that are called to playback audio streams
	// each device is given exactly one decoder as we may want to play multiple tracks seperatly
	std::vector<ma_device*> devices;
	// which indicies can be assigned to new sounds, it being empty means that a new sound will append the list of devices
	std::vector<size_t> availableIndices;

	// added to the userdata of a device so that it can reference its decoder and an AudioProcessor
	struct DeviceData {
		size_t id; // which device it is in its processor
		AudioProcessor* processor;
		ma_decoder* decoder;
	};
	std::vector<DeviceData*> deviceDatas;


	//the last energies that were processed by the playbacking devices
	std::vector<float> energies;
	// 1/seconds it takes for an ergy to go from 1 to 0;
	std::vector<float> decayRates;

	// information as to how an element in getEnergy should be produced
	class EnergyMapper {
	public:
		size_t soundId; // which sound energy is based on
		float minFreq; // lowest frequency of sample to consider
		float maxFreq; // highest frequency of sample to consider
		float minAmp; //min size of the signal in [0, 1]. all values under reated as 0
		float maxAmp; //max size of the signal in [0, 1]
		ma_format format; // format and descriptors of sound
		ma_uint32 channels;
		ma_uint32 sampleRate;
		ma_lpf1* lowpass; 
		ma_hpf1* highpass; // filter objects that apply the filters

		void reinitialise(float minF, float maxF, float minA, float maxA);

		EnergyMapper(size_t id, float minF, float maxF, float minA, float maxA, ma_format form, ma_uint32 chan, ma_uint32 samp);
		~EnergyMapper();
	};

	//keep a list of them
	std::vector<EnergyMapper*> energyMappers;
	// responsible for decreasing all energies even if devices are paurse
	ma_thread decayThread;
	struct DecayThreadData {
		AudioProcessor *processor;
		ma_fence *fence;
		bool end; //signal the decay thread that it should close
	};
	DecayThreadData decayThreadData;
	//creats a ma_device set to play audio from a given fileName, initialised
	Result createDeviceFromFile(ma_device *device, const char* fileName, size_t id);

	//Calculates the energy from the next sequence of PCM frames
	static float calcEnergy(const void* framesIn, ma_uint64 nFrames, EnergyMapper *mapper);

	//decrease all energys in a AudioProcessor by their desired amount
	static void decayEnergyPasses(void* decayThreadData);

	//used to read and write audio data from and to buffers from a device
	static void dataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
	//frees device and accociated custom user data
	static void destroyDevice(ma_device* device);

	//frees memory accosiated with stored devices
	void destroyAllDevices();
	void destroyAllEnergyMappers();
};
}


#endif // AVIS_AUDIO_PROCESSOR