#include "AudioVisualiser.h"

#include <iostream>
#include <glm/gtc/constants.hpp>
//debug only
#ifdef _DEBUG
#define MIN(a, b) a < b ? a : b
//lmao
#endif // _DEBUG


using namespace AVis;
std::string App::audioFile = "C:\\Users\\resos\\Documents\\lmms\\projects\\retreat.ogg";


const float App::CameraRotSpeed = glm::pi<float>() * 2.5;
const float App::CameraMoveSpeed = 20;

void App::run() {
	initApp();
	mainLoop();
	cleanup();
}

void App::setPeakType(uint32_t peakInd, Peak::Type newType) {
	peaks[peakInd].type = newType;
}

void App::setPeakPosition(uint32_t peakInd, float x, float z) {
	peaks[peakInd].point = { x, z };
}

void App::setPeakHeight(uint32_t peakInd, float h) {
	peaks[peakInd].maxHeight = h;
}

void App::setPeakWidth(uint32_t peakInd, float w) {
	//peaks[peakInd] = w;
}

Peak* App::getPeakData() {
	return peaks.data();
}

size_t App::getNumPeaks() {
	return peaks.size();
}

// try to load an audio file for the visualiser to. debug only. one sound
bool App::loadAudioFile(const char* filePath) {
	if (audioFileLoaded) {
		return(audioProcessor->changeAudioFile(0, filePath) == AudioProcessor::Result::SUCCESS);
		
	}
	else {
		size_t soundInd = (audioProcessor->addSoundFromFile(filePath));
		audioFileLoaded = (soundInd != UINT16_MAX);
		if (audioFileLoaded) {
			initPeaks(soundInd);
		}
		return audioFileLoaded;
	}
}

void App::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
	(app->visualiser)->resizeImage(width, height);
}

void App::keyInput(GLFWwindow* window, int key, int scancode, int action, int mods) {
	App *app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));

	if (action & (GLFW_PRESS)) {
		switch (key) {
			case GLFW_KEY_R:
				app->audioProcessor->playAll(true);
				break;
			case GLFW_KEY_SPACE:
				//toggle pause/play
				if (app->audioProcessor->isPlaying()) {
					app->audioProcessor->stopAll(false);
				}
				else {
					app->audioProcessor->playAll(false);
				}
			case GLFW_KEY_T:
				//stop
				app->audioProcessor->stopAll(true);
				break;
		}
	}
	app->cameraMovementKeyInput(key, scancode, action, mods);
}

void App::cameraMovementKeyInput(int key, int scancode, int action, int mods) {
	bool doRotate = false;
	bool doTranslate = false;
	float rotates[2] = { 0.0f };
	float translates[3] = {0.0f};
	if (action & (GLFW_PRESS | GLFW_REPEAT)) {
		switch (key) {
		//camera rotation
		case GLFW_KEY_UP:
			rotates[1] = CameraRotSpeed * timeDelta;
			doRotate = true;
			break;
		case GLFW_KEY_DOWN:
			rotates[1] = -CameraRotSpeed * timeDelta;
			doRotate = true;
			break;
		case GLFW_KEY_LEFT:
			rotates[0] = CameraRotSpeed * timeDelta;
			doRotate = true;
			break;
		case GLFW_KEY_RIGHT:
			rotates[0] = -CameraRotSpeed * timeDelta;
			doRotate = true;
			break;
		// movement
		case GLFW_KEY_W:			
			translates[2] = -CameraMoveSpeed * timeDelta;
			doTranslate = true;
			break;
		case GLFW_KEY_S:
			translates[2] = CameraMoveSpeed * timeDelta;
			doTranslate = true;
			break;
		case GLFW_KEY_A:
			translates[0] = -CameraMoveSpeed * timeDelta;
			doTranslate = true;
			break;
		case GLFW_KEY_D:
			translates[0] = CameraMoveSpeed * timeDelta;
			doTranslate = true;
			break;
		case GLFW_KEY_SEMICOLON:
			visualiser->printThing();
			break;
		}
	}
	if (doRotate) {
		visualiser->rotateCamera(rotates[0], rotates[1]);
	}
	if (doTranslate) {
		visualiser->translateCamera(translates[0], translates[1], translates[2]);
	}
}

void App::initApp() {
	startTime = std::chrono::steady_clock::now();
	
	// glfw objects
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	window = glfwCreateWindow(DefWindowWidth, DefWindowHeight, "Vulkan", nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	glfwSetKeyCallback(window, keyInput);
	
	//create and initialise visualiser
	visualiser = new Visualiser();
	visualiser->initVulkan(window, DefWindowWidth, DefWindowHeight);

	// "C:\\Users\\resos\\Music\\Happy Hour - SSN - Peppered OST.mp3";
	// "C:\\Users\\resos\\Music\\itsumi_ssn_peppered_ost_4367377276624181615.mp3";
	// 
	audioProcessor = new AudioProcessor;

	gui = new GUI(this, window, visualiser);

	//audioProcessor->playAll();
}

void App::initPeaks(size_t audioInd) {
	size_t nPeaks = 16;

	float minFreq = 20.0f;
	float maxFreq = 7000.0f;

	float base = std::powf(maxFreq / minFreq, 1.0f / nPeaks);
	float value = base;
	for (size_t i = 0; i < nPeaks; i++) {
		float nextValue = value * base;
		audioProcessor->setTrackEnergy(i, audioInd, value, nextValue, 0.0, 0.4);
		value = nextValue;
	}
}

void App::queueRendererResize() {
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	while (width == 0 || height == 0) {
		//do nothing on minimization
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}
	visualiser->resizeImage(width, height);
}

void App::updatePeakEnergies(std::vector<float> energies) {
#ifdef _DEBUG
	if (energies.size() != peaks.size()) {
		std::cerr << "trying to update " << energies.size() << " energies, but there are " << peaks.size() << "peaks.\n";
		energies.resize(MIN(energies.size(), peaks.size()));
	}
#endif // _DEBUG
	for (size_t i = 0; i < energies.size(); i++) {
		peaks[i].energy = energies[i];
	}
}

void App::mainLoop() {
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		
		//audioStep
		std::vector<float> newEnergies = audioProcessor->getEnergies();
		updatePeakEnergies(newEnergies);
		visualiser->updatePeaks(peaks);

		if (visualiser->requestingResize) {
			queueRendererResize();
		}
		
		auto nowTime = std::chrono::steady_clock::now();
		float newTime = std::chrono::duration<float>(nowTime - startTime).count();
		timeDelta = newTime - time;
		gui->process(timeDelta);

		visualiser->drawFrame();

		time = newTime;
	}
}

void App::cleanup() {
	visualiser->cleanup();

	glfwDestroyWindow(window);

	glfwTerminate();
	std::cout << "Ran for " << time << " seconds." << std::endl;
	//dont forget not to leak memory
	delete visualiser;
	delete audioProcessor;
}