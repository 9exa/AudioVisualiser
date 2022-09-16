#ifndef AVIS_APP
#define AVIS_APP


//#define glfw_include_vulkan
//#include <glfw/glfw3.h>
//#include <vulkan/vulkan.h>

#include <vector>
#include <string>
#include <chrono>

#include "VisualiserTypes.h"
#include "Visualiser.h"
#include "AudioProcessor.h"
#include "GUI.h"
namespace AVis {

class App {
    // An audio visualizer using Vulkan graphics Api
public:
    void run();

    float time;

    //setup the AudioVisualiser and play an audio file
    // make it console searchable because I'm lazy

    static std::string audioFile;
    bool audioFileLoaded = false;
    
    inline void setPeakType(uint32_t peakInd, Peak::Type newType);

    inline void setPeakPosition(uint32_t peakInd, float x, float z);

    inline void setPeakHeight(uint32_t peakInd, float h);

    inline void setPeakWidth(uint32_t peakInd, float w);


    void addPeak();
    void removePeak(uint32_t peakInd);

    // give part of application the ability to edit peak data seperately but not the ability to 
    // change the number of peaks
    Peak *getPeakData();
    size_t getNumPeaks();

    // try to load an audio file for the visualiser to
    bool loadAudioFile(const char* filePath);


private:
    const uint32_t DefWindowWidth = 1280;
    const uint32_t DefWindowHeight = 800;

    std::chrono::time_point<std::chrono::steady_clock> startTime;
    float timeDelta; // time between now and last frame

    // window that pops up on screen and displays content
    GLFWwindow* window = nullptr;

    // Visual renderer of audio data
    Visualiser* visualiser = nullptr;

    GUI* gui = nullptr;

    // AudioPlayer that also sends data of peaks
    AudioProcessor* audioProcessor = nullptr;

    // Rate that camera rotates when corresponding buttons are pressed in radians per second;
    static const float CameraRotSpeed;
    static const float CameraMoveSpeed;

    // We're also storing peak info here so entire application has access to them
    std::vector<Peak> peaks {
            {Peak::Type::Quadratic, {-6.0f, -3.0f}, 2.0, 0.5f, Colors::NeonBrightRed},
            {Peak::Type::Quadratic, {-2.0f, -3.0f}, 2.0, 0.0f, Colors::NeonBrightRed},
            {Peak::Type::Quadratic, {2.0f, -3.0f}, 2.0, 0.0f, Colors::NeonBrightRed},
            {Peak::Type::Quadratic, {6.0f, -3.0f}, 2.0, 0.0f, Colors::NeonBrightRed},
            {Peak::Type::Quadratic, {-6.0f, -1.0f}, 2.0, 0.0f, Colors::NeonBrightRed},
            {Peak::Type::Quadratic, {-2.0f, -1.0f}, 2.0, 0.0f, Colors::NeonBrightRed},
            {Peak::Type::Quadratic, {2.0f, -1.0f}, 2.0, 0.0f, Colors::NeonBrightRed},
            {Peak::Type::Quadratic, {6.0f, -1.0f}, 2.0, 0.0f, Colors::NeonBrightRed},
            {Peak::Type::Quadratic, {-6.0f, 1.0f}, 1.0, 0.0f, Colors::NeonBrightRed},
            {Peak::Type::Quadratic, {-2.0f, 1.0f}, 1.0, 0.0f, Colors::NeonBrightRed},
            {Peak::Type::Quadratic, {2.0f, 1.0f}, 1.0, 0.0f, Colors::NeonBrightRed},
            {Peak::Type::Quadratic, {6.0f, 1.0f}, 1.0, 0.0f, Colors::NeonBrightRed},
            {Peak::Type::Sphere, {-6.0f, 3.0f}, 1.0, 0.0f, Colors::NeonPink},
            {Peak::Type::Sphere, {-2.0f, 3.0f}, 1.0, 0.0f, Colors::NeonPink},
            {Peak::Type::Sphere, {2.0f, 3.0f}, 1.0, 0.0f, Colors::NeonPink},
            {Peak::Type::Sphere, {6.0f, 3.0f}, 1.0, 0.0f, Colors::NeonPink}
    };


    //function listening to glfw resize event
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    // listen to keyboard inputs
    static void keyInput(GLFWwindow* window, int key, int scancode, int action, int mods);
    // subprocesses for keyInput
    void cameraMovementKeyInput(int key, int scancode, int action, int mods);

    // initialise members for a runtime
    void initApp();

    void initPeaks(size_t soundInd);

    // wait match the renderer size to the current window State
    void queueRendererResize();

    // make all peaks change energies
    void updatePeakEnergies(std::vector<float> energies);

    // The main, continuous operations of the app.
    void mainLoop();

    //cleans up processes on a runtime end
    void cleanup();
};

}; //namespace AudioVisualiser

#endif // AV_APP



