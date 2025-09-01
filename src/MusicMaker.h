#pragma once

#include <nlohmann/json.hpp>
#include <sol/sol.hpp>

#include "MIDI.h"
#include "MelodyMaker.h"

#include "algo/BestOrder.h"
#include "algo/ChordDetector.h"
#include "algo/KeyDetector.h"

#include "data/DrumPattern.h"
#include "data/Instrument.h"

#include "game/GameBridge.h"


// Wraps all variable parts of the music generation into one object
struct MusicState {
	double intensity		= 0.0;
	Scale scale				= Scale::IONIAN;
	double tempoMultiplier	= 1.0;
	std::string leadStyle	= "Sustain";
	int leadLayers			= 1;
	int chordLayers			= 1;
	std::string bassStyle	= "Sustain";
	DrumPattern drumPattern	= DrumPattern::NONE;
};

struct ThemePlaybackState {
	double msOffset = 0.0;
};


class MusicMaker {
public:
	MusicMaker() = default;

	void start();

private:
	// FUNCTIONS
	// Lua
	void bindMusicFunctions();
	void loadLuaRules();
	void loadLuaLogic();
	void loadLuaFileSafe(const std::string &path, const char *tag);
	void validateAllRules();

	// GameState
	void startGameStateThread();
	void handleGameState(nlohmann::json& parsed, sol::table& gameState);

	// MIDI
	void preloadMIDIFile(const std::string &path);
	smf::MidiFile getCachedMIDI(const std::string& path) const;
	int getNextAvailableChannel();
	void activateMIDITheme(const std::string& path, int program);
	void deactivateMIDITheme(const std::string &path);

	// Events
	std::shared_ptr<ScheduledEvent> pollNextEventWithTiming(double melodyTime);

	void scheduleNote(Note note, Clock::time_point startTime, double duration, ActiveInstrument instrument);
	void scheduleMelody(const std::vector<std::shared_ptr<ScheduledEvent>> &schedule);
	void scheduleBass(const Chord& nextChord, Clock::time_point mstAbs);
	void scheduleDrums(Clock::time_point mstAbs);

	void playChordTransition(const Chord &lastChord, const Chord &nextChord);

	void play();
	void pause();
	void resume();


	// VARIABLES
	// Game
	GameBridge gb;

	std::mutex gameStateMutex;
	std::optional<nlohmann::json> currentGameState;
	std::atomic<bool> stopReceiver{false};

	sol::state lua;
	sol::function onStart, onUpdate;

	// General music
	MIDI midi;
	MelodyMaker mm;
	Mode mode = Mode::IDLE;

	// Music state
	MusicState musicState;

	// MIDI
	std::string mainMIDIFilePath;
	std::unordered_map<std::string, smf::MidiFile> preloadedMIDICache;
	std::unordered_map<std::string, ActiveInstrument> activeThemes;
	std::unordered_map<std::string, ThemePlaybackState> themePlaybackStates;

	// Melody generation
	Melody melody{};  // Input melody provided by the user
	int markovOrder{};
	bool autoMarkov{};

	// Playback
	std::vector<ScheduledPlaybackEvent> playbackQueue;
	Clock::time_point playStartTime;
	Clock::time_point pauseTime;
	std::atomic<bool> isPaused{false};
	std::atomic<bool> wasJustResumed{false};

	// Live recording
	std::atomic<bool> isFirstNote{true};

	// Timing
	TimeSignatureInfo tsInfo;
	double originalBpm = 0.0;
	double currentBpm = originalBpm;

	std::unordered_map<DrumPattern, Pattern> drumPatterns;
};
