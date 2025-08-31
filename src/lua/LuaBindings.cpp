#include "MusicMaker.h"

using namespace std;


void MusicMaker::bindMusicFunctions() {
	lua.open_libraries(
		sol::lib::base,
		sol::lib::math,
		sol::lib::table
	);

	// Expose `music` table to Lua
	sol::table musicTable = lua.create_named_table("music");


	// MIDI
	// set_main_midi
	musicTable.set_function("set_main_midi", [this](const string& path) {
		mainMIDIFilePath = path;
		cout << "[Lua] Set main MIDI file to \"" << path << "\"\n";
	});

	// use_auto_markov
	musicTable.set_function("use_auto_markov", [this](const bool enable) {
		autoMarkov = enable;
		cout << "[Lua] Set Auto-Markov to " << enable << endl;
	});

	// set_markov_order
	musicTable.set_function("set_markov_order", [this](const int order) {
		if (autoMarkov) return;
		markovOrder = order;
		cout << "[Lua] Set Markov order to " << order << endl;
	});

	// preload_midi
	musicTable.set_function("preload_midi", [this](const string& path) {
		preloadMIDIFile(path);
		cout << "[Lua] Preloaded MIDI file \"" << path << "\"\n";
	});

	// add_midi
	musicTable.set_function("add_midi", [this](const string& path, const int program) {
		activateMIDITheme(path, program);
		cout << "[Lua] Activated MIDI theme \"" << path << "\" with instrument " << program << endl;
	});

	// remove_midi
	musicTable.set_function("remove_midi", [this](const string& path) {
		deactivateMIDITheme(path);
		cout << "[Lua] Deactivated MIDI theme \"" << path << "\"\n";
	});


	// EFFECTS
	// set_intensity
	musicTable.set_function("set_intensity", [this](const double intensity) {
		const double clamped = clamp(intensity, 0.0, 1.0);
		if (clamped == musicState.intensity) return;
		musicState.intensity = clamped;
		cout << "[Lua] Set intensity to " << clamped << endl;
	});

	// set_scale
	musicTable.set_function("set_scale", [this](const string& newScale) {
		const Scale parsed = getScaleFromName(newScale);
		if (parsed == musicState.scale) return;
		musicState.scale = parsed;
		cout << "[Lua] Set scale to \"" << newScale << "\"\n";
	});

	// set_tempo_multiplier
	musicTable.set_function("set_tempo_multiplier", [this](const double factor) {
		if (factor == musicState.tempoMultiplier) return;
		musicState.tempoMultiplier = factor;
		cout << "[Lua] Set tempo multiplier to " << factor << endl;
	});

	musicTable.set_function("set_lead_style", [this](const string& style) {
		if (style == musicState.leadStyle) return;
		musicState.leadStyle = style;
		cout << "[Lua] Set lead style to \"" << style << "\"\n";
	});

	// set_lead_layers
	musicTable.set_function("set_lead_layers", [this](const int octaves) {
		if (octaves == musicState.leadLayers) return;
		musicState.leadLayers = octaves;
		cout << "[Lua] Set lead layers to " << octaves << endl;
	});

	// set_chord_layers
	musicTable.set_function("set_chord_layers", [this](const int octaves) {
		if (octaves == musicState.chordLayers) return;
		musicState.chordLayers = octaves;
		cout << "[Lua] Set chord layers to " << octaves << endl;
	});

	// set_bass_style
	musicTable.set_function("set_bass_style", [this](const string& style) {
		if (style == musicState.bassStyle) return;
		musicState.bassStyle = style;
		cout << "[Lua] Set bass style to \"" << style << "\"\n";
	});

	// set_drum_pattern
	musicTable.set_function("set_drum_pattern", [this](const string& patternName) {
		const DrumPattern parsed = getDrumPattern(patternName);
		if (parsed == musicState.drumPattern) return;
		musicState.drumPattern = parsed;
		cout << "[Lua] Set drum pattern to \"" << patternName << "\"\n";
	});
}


void MusicMaker::loadLuaRules() {
	lua.script_file(LUA_RULES_SCRIPT_PATH);
}


void MusicMaker::loadLuaLogic() {
	lua.script_file(LUA_LOGIC_SCRIPT_PATH);

	onStart   = lua["on_start"];
	onUpdate  = lua["on_update"];

	if (!onStart)
		cerr << "[Lua] Warning: Required Lua on_start function missing or invalid\n";
	if (!onUpdate)
		cerr << "[Lua] Warning: Required Lua on_update function missing or invalid\n";
}
