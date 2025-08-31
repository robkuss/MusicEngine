#include "MusicMaker.h"

#include <cfloat>

using namespace std;
using namespace smf;


void MusicMaker::preloadMIDIFile(const string& path) {
	const string fullPath = INPUT_DIR + path;

	// Avoid reloading if already cached
	if (preloadedMIDICache.contains(path)) {
		cout << "[MusicMaker] MIDI file already preloaded: " << path << endl;
		return;
	}

	MidiFile midiFile;
	if (!midiFile.read(fullPath)) {
		cerr << "[MusicMaker] Failed to preload MIDI file: " << fullPath << endl;
		return;
	}

	// Analyze MIDI
	midiFile.doTimeAnalysis();
	midiFile.linkNotePairs();
	midiFile.sortTracks();		// Ensure chronological order (sort by tick time) just in case
	midiFile.absoluteTicks();	// Ensure we're in absolute tick mode

	preloadedMIDICache[path] = move(midiFile);
}

MidiFile MusicMaker::getCachedMIDI(const string& path) const {
	const auto it = preloadedMIDICache.find(path);
	if (it == preloadedMIDICache.end()) {
		cerr << "[MusicMaker] MIDI not preloaded: " << path << endl;
		exit(1);  // or throw
	}
	return it->second;
}


int MusicMaker::getNextAvailableChannel() {
	// Skip 0–2. These are reserved for the core instruments (0 = LEAD, 1 = CHORDS, 2 = BASS)
	for (int ch = 3; ch < 16; ++ch) {
		if (ch == 9) continue;  // Skip 9 (DRUMS)

		const bool taken = ranges::any_of(activeThemes, [ch](const auto& pair) {
			return pair.second.channel == ch;
		});

		if (!taken) return ch;
	}
	return -1;  // No channels available
}


void MusicMaker::activateMIDITheme(const string& path, const int program) {
	if (activeThemes.contains(path)) return;  // Already active

	const int channel = getNextAvailableChannel();
	if (channel == -1) {
		cerr << "[MusicMaker] No available channel for theme: " << path << endl;
		return;
	}

	midi.selectProgram(channel, program);

	activeThemes[path] = ActiveInstrument{
		.channel  = channel,
		.program  = program,
		.velocity = 127  // TODO
	};

	cout << "[MusicMaker] Assigned channel " << channel << " for theme '" << path
		 << "' (program " << program << " at default velocity 100)" << endl;
}

void MusicMaker::deactivateMIDITheme(const string& path) {
	if (activeThemes.erase(path) > 0) {
		cout << "[MusicMaker] Removed theme instrument for '" << path << "' and freed its channel\n";
	}
}
