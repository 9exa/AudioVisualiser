#ifndef AVIS_GUI
#define AVIS_GUI

#include "GLFW/glfw3.h"
#include "../external/imgui/imgui.h"
#include "../external/imfilebrowser.h"

namespace AVis {
//forward declare circular dependecies on app class and renderer
class App;
class Visualiser;

class GUI {
	// Displays and listens for signals to gui of the application
	// Competely unaware of vulkan but does have access to the renderer for redrawing imgui
	// Also has access to window in case of resize
public:
	GUI(App* a, GLFWwindow *w, Visualiser* r);
	
	void process(float delta);
private:
	static const std::vector<std::string> MusicExtensions;

	App *app;
	GLFWwindow* window;
	Visualiser* renderer;

	ImGui::FileBrowser fileDialog;

	void presentPeakEditor();
	void presentFileDialog();

};

};

#endif

