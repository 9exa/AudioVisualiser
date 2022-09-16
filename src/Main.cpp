// AudioVisualiser.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

//#include "pch.h"
#include <iostream>
#include <cstring>
#include <string>
#include <filesystem>
namespace fs = std::filesystem;

#include "AudioVisualiser.h"

static char* getFileExtension(const char* str) {
    int dotPos = -1;
    int ind = 0;
    while (str[ind] != '\0') {
        if (str[ind] == '.')
            dotPos = ind;
        ind++;
    }

    if (dotPos != -1) {
        int outSize = ind - (dotPos);
        char* output = new char[outSize];// (char*)malloc(outSize * sizeof(char));
        strcpy_s(output, outSize, str+dotPos + 1);
        return output;
    }
    else {
        char* output = new char[1];//(char*)malloc(sizeof(char));
        strcpy_s(output, 1, "");
        return output;
    }
}

static bool isValidAudioFileName(const char* fileName) {
    static const char* AcceptableExtensions[3] = {
        "mp3", "ogg", "wav"
    };
    char* ext = getFileExtension(fileName);
    bool result = false;
    for (size_t i = 0; i < 3; i++) {
        result |= (strcmp(ext, AcceptableExtensions[i]) == 0);
    }
    delete ext;
    return result;
}

int main(const int argc, const char *argv[])
{
    AVis::App app;
    /*
    if (argc > 1 && isValidAudioFileName(argv[1])) {
        // make the app run selected file if specified and valid
        AVis::App::audioFile = argv[1];
    }
#ifndef _DEBUG
    else {
        // Otherwise look in current directory for first file with valid extension
        std::string audioFileString;
        for (const auto& entry : fs::directory_iterator(".")) {
            if (isValidAudioFileName(entry.path().string().data())) {
                audioFileString = entry.path().string();
                break;
            }
        }
        if (audioFileString.empty()) {
            std::cerr << "No valid audio file in this directory. \
                            Consider running thnis program with a path to the ogg/mp3/wav file." << std::endl;
            return EXIT_FAILURE;
        }
        else {
            AVis::App::audioFile = audioFileString.c_str();
        }
}
#endif // ! NDEBUG
    */
    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
