#include "GUI.h"


#include "AudioVisualiser.h"
#include <iostream>

using namespace AVis;

GUI::GUI(App* a, GLFWwindow *w, Visualiser* r) : app(a), window(w), renderer(r) {
    
};

void GUI::process(float delta) {
	renderer->newImguiFrame();
    bool show_demo_window = true;
    bool show_another_window = false;
    static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    
    presentPeakEditor();

	ImGui::Render();
}

const std::vector<std::string> GUI::MusicExtensions = {".ogg", ".mp3", ".wav"};

void GUI::presentPeakEditor() {
    Peak* peakData = app->getPeakData();
    size_t nPeaks = app->getNumPeaks();
    //std::cout << fileDialog.IsOpened() << std::endl;

    ImGui::Begin("Peak editor");
    if (ImGui::Button("Change sound")) {
        fileDialog.SetTitle("Choose sound file. File Explorer By  AirGuanZ - https://github.com/AirGuanZ/imgui-filebrowser");
        fileDialog.SetTypeFilters(MusicExtensions);
        fileDialog.Open();
    }
    ImGui::Text("R - Restart song (if any is loaded)");
    ImGui::Text("WASD - move around");
    ImGui::Text("Arrows - rotate camera");

    presentFileDialog();

    for (size_t i = 0; i < nPeaks; i++) {
        ImGui::PushID(i);
        ImGui::Text("Peak %d", i + 1);

        // The type dropdown
        if (ImGui::BeginCombo("Type", Peak::TypeNames[peakData[i].type])) {
            for (uint32_t j = 0; j < Peak::Type::NTypes; j++) {
                const char* name = Peak::TypeNames[j];
                // new type selected
                if (ImGui::Button(name)) {
                    *&peakData[i].type = (Peak::Type)j;
                }
            }
            ImGui::EndCombo();
        }
        

        ImGui::SliderFloat("Height", &(peakData[i].maxHeight), Peak::MinHeight, Peak::MaxHeight);
        ImGui::SliderFloat2("Position", &(peakData[i].point.x), Peak::MinGridPos, Peak::MaxGridPos);
        ImGui::ColorEdit3("Colour", &(peakData[i].color[0]));

        ImGui::PopID();
    }
    ImGui::End();
}

void GUI::presentFileDialog() {
    fileDialog.Display();
    if (fileDialog.HasSelected())
    {
        //std::cout << "Selected filename" << fileDialog.GetSelected().string() << std::endl;
        if (app->loadAudioFile(fileDialog.GetSelected().string().data())) {
            fileDialog.ClearSelected();
        }
        else {
            ImGui::OpenPopup("File could not be Opened");
        }
    }
}
