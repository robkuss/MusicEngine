#pragma once

#include <string>

#include <filesystem>
#include <unordered_map>
#include <vector>

#include <glad/glad.h>
#include "GLFW/glfw3.h"

#include "util/Util.h"

#include "game/GameData.h"
#include "game/Rule.h"


// Simple global state machine
enum class GUIState {
	TitleScreen,
	RuleDef,
	MusicGen
};


class GUI {
public:
	GUI(const std::string &title, int width, int height);

	void start();

private:
	int width, height;
	std::string title;

	GUIState guiState = GUIState::TitleScreen;

	GameData selectedGame;
	std::string selectedMidi;

	std::unordered_map<TriggerKey, Rule, TriggerKeyHash> rules;
	std::vector<TriggerKey> ruleOrder;
	int selectedRuleIdx = -1;

	std::vector<std::string> midiFiles = scanMidiFiles();

	bool autoMarkov  = true;
	int  markovOrder = 1;

	void titleScreen();
	void ruleDef();
	void musicGen();

	char outPath[260];
	void exportLua(const std::string& path);
	std::string findRulesLua() const;
	bool loadRulesFromLua(const std::string& path, std::string& outMessage);
};
