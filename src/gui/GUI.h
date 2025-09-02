#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <glad/glad.h>
#include "GLFW/glfw3.h"

#include "util/Util.h"


// Rule data per mob
struct Rule {
	// Theme group
	bool addTheme = false;
	std::string theme;   // file name from /input
	int themeIdx = -1;   // index into midiFiles
	int instrument = 0;  // 0..127

	// Optional rule parameters
	std::optional<float> intensity;          // [0..1]
	std::optional<std::string> scale;        // one of kScales
	std::optional<float> tempo_multiplier;   // >= 0
	std::optional<std::string> lead_style;   // one of kLeadStyles
	std::optional<int> lead_layers;          // >= 1
	std::optional<int> chord_layers;         // >= 1
	std::optional<std::string> bass_style;   // one of kBassStyles
	std::optional<std::string> drum_pattern; // one of kDrumPatterns
};


// Simple global state machine
enum class GUIState {
	TitleScreen, Project
};


class GUI {
public:
	GUI(const std::string &title, int width, int height);

	void start();

private:
	int width, height;
	std::string title;

	GUIState guiState = GUIState::TitleScreen;

	std::string selectedGame = "Minecraft";
	std::string selectedMidi;	// from dropdown
	char customMidi[256] = "";	// manual input buffer

	std::unordered_map<std::string, Rule> rules;
	std::vector<std::string> midiFiles = scanMidiFiles();

	static void errorCallback(int code, const char *msg);
	static void windowSizeCallback(GLFWwindow *window, int width, int height);

	void titleScreen();
	void project();
};
