/*
#include "../src/MusicMaker.h"

#include "RandomLuaRules.h"
#include "RandomMIDICreator.h"
#include "TestCSV.h"

using namespace std;
using namespace smf;

constexpr bool INIT_DEBUG = false;
constexpr bool DEBUG_JSON = false;

constexpr bool OFFLINE_MODE = false;  // Doesn't require connection to a game to generate music

long long autoMs;
Clock::time_point t_start;


int main() {
	constexpr int iterations = 10;
	const vector lengths	 = {2, 5, 10, 25, 100};  // in measures

	for (const int length : lengths) {
		for (int i = 0; i < iterations; i++) {
			createRandomMIDISequencesFor(lengths);

			const std::string file = std::format("{}.mid", length);
			cout << file << endl;

			// createRandomLuaRules(1, file, true);

			// Start initialization performance tests
			autoMs = 0;
			MusicMaker().startPT(file);
		}
	}
}


void MusicMaker::startPT(const std::string& file) {
	preloadMIDIFile(file);

	// Get input training MIDI
	MidiFile mainMIDIFile = getCachedMIDI(file);

	// Extract bpm and time signature
	tsInfo = extractTimeSignatureInfo(mainMIDIFile);
	originalBpm = tsInfo.bpm;

	// Generate the drum pattern map
	drumPatterns = getDrumPatterns(tsInfo);

	// Extract melody
	processMidiEvents(mainMIDIFile, tsInfo, [&](
		const Note note,
		const Clock::time_point eventStartTime,
		const double duration
	) {
		const auto event = make_shared<FixedEvent>(
			note,
			getMTP(playStartTime, eventStartTime, tsInfo),
			duration
		);

		melody.events.push_back(event);
	});

	KeyDetectionResult result;
	updateMelodyMetadata(melody, INIT_DEBUG ? &result : nullptr);

	// Set Markov chain order (a.k.a. lookbehind)
	const auto t_auto_begin = Clock::now();
	determineBestOrder(melody, mode);
	autoMs = timeBetween(t_auto_begin, Clock::now());

	// Start generating music
	printf("[Init] Auto Markov = %lld ms\n", autoMs);

	logPerformanceCSV("perf_results.csv", autoMs, melody.events.size());
}
*/
