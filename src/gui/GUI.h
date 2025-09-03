#pragma once

#include <string>

#include <filesystem>
#include <unordered_map>
#include <vector>

#include <glad/glad.h>
#include "GLFW/glfw3.h"

#include "MusicMaker.h"
#include "game/GameData.h"
#include "game/Rule.h"
#include "GUILogger.h"
#include "util/Util.h"


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
	std::string title;
	int width, height;

	GLFWwindow* window = nullptr;

	// MusicGen runtime
	std::unique_ptr<MusicMaker> music;
	std::thread musicThread;
	std::atomic<bool> musicRunning{false};
	std::atomic<bool> musicPaused{false};

	// RuleDef
	GameData selectedGame;

	std::vector<std::string> midiFiles = scanMidiFiles();
	std::string selectedMidi;

	bool autoMarkov  = true;
	int  markovOrder = 1;

	std::unordered_map<TriggerKey, Rule, TriggerKeyHash> rules;
	std::vector<TriggerKey> ruleOrder;
	int selectedRuleIdx = -1;

	char outPath[260];
	void exportLua(const std::string& path);
	std::string findRulesLua() const;
	bool loadRulesFromLua(const std::string& path, std::string& outMessage);

	// GUI States
	GUIState guiState = GUIState::TitleScreen;
	void titleScreen();
	void ruleDef();
	void musicGen();

	// Helper to stop/join the thread safely
	void stopMusicIfRunning();

	// Logging / redirection
	GuiLogger guiLog;
	std::unique_ptr<GuiLogStreamBuf> coutRedirect;
	std::unique_ptr<GuiLogStreamBuf> cerrRedirect;
	std::streambuf* oldCoutBuf = nullptr;
	std::streambuf* oldCerrBuf = nullptr;
	bool autoScrollLog = true;
	size_t lastLogVersion = 0;  // for auto-scroll
};
