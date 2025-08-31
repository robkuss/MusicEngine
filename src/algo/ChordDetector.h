#pragma once

#include "../data/Scale.h"

#include <bits/ranges_algo.h>

struct ChordType {
	std::string quality;
	std::vector<int> intervals;	 // e.g., {0, 4, 7} for major
};

struct Chord {
	Note root;
	ChordType type;
	std::string name;
};

inline const std::vector<ChordType> chordTypes = {
	{"major", {0, 4, 7}},
	{"minor", {0, 3, 7}},
	{"sus2",  {0, 2, 7}},
	{"sus4",  {0, 5, 7}},
	{"dim",   {0, 3, 6}},
	{"aug",   {0, 4, 8}},
	{"maj7",  {0, 4, 7, 11}},
	{"min7",  {0, 3, 7, 10}},
	{"7",     {0, 4, 7, 10}},
	{"major add9", {0, 2, 4, 7}},
	{"minor add9", {0, 2, 3, 7}}
};

inline std::string basicQuality(const Chord& chord) {
	auto& intervals = chord.type.intervals;
	auto has = [&](const int i) {
		return std::ranges::find(intervals, i) != intervals.end();
	};

	if (has(4)) {
		if (has(7)) return "major";
		if (has(8)) return "aug";
	}
	if (has(3)) {
		if (has(7)) return "minor";
		if (has(6)) return "dim";
	}
	if (has(2) || has(5)) {
		return "sus";
	}
	return "none";
}


inline std::vector<Chord> diatonicChords{};

inline void generateDiatonicChords(const Note key, const Scale mode) {
	diatonicChords.clear();
	const auto scale = buildScale(key, mode);

	// Iterate through each note in the scale to use as the chord root
	for (size_t i = 0; i < 7; ++i) {
		const Note root = scale[i];

		// Try each chord type
		for (const auto& [quality, intervals] : chordTypes) {
			bool allInScale = true;

			// Check if all chord tones (based on intervals) are in the scale
			for (const int interval : intervals) {
				Note note = (root + interval) % 12;

				// If any chord tone is not in the scale, discard this chord
				if (const bool found = std::ranges::any_of(scale, [note](const Note s) {
					return s % 12 == note;
				}); !found) {
					allInScale = false;
					break;
				}
			}

			// If all chord tones fit within the scale, add the chord
			if (allInScale) {
				diatonicChords.emplace_back(
					root,
					ChordType(quality, intervals),
					getNoteName(root, false) + " " + quality
				);
			}
		}
	}
}

inline std::vector<std::pair<Chord, int>> scoreChords(const std::vector<Note>& melodySegment, const Scale scale) {
	// Count the occurrences of each pitch class (0–11)
	std::array<int, 12> pitchCount{};
	for (const Note note : melodySegment) {
		pitchCount[note % 12]++;
	}

	// Calculate the score (how many chord notes appear in the melody segment)
	std::vector<std::pair<Chord, int>> scoredChords;
	for (const auto& chord : diatonicChords) {
		int score = 0;
		for (const Note interval : chord.type.intervals) {
			const Note chordNote = (chord.root + interval) % 12;
			score += pitchCount[chordNote];
		}

		// Mode-aware chord bias
		if (scale == Scale::PHRYGIAN || scale == Scale::LOCRIAN) {
			if (chord.type.quality == "major")
				score -= 2;  // Penalize bright chords in dark modes
			else if (chord.type.quality == "minor" || chord.type.quality == "dim")
				score += 1;  // Slightly favor darker harmonies
		} else if (scale == Scale::LYDIAN || scale == Scale::IONIAN) {
			if (chord.type.quality == "minor" || chord.type.quality == "dim")
				score -= 1;  // Slight penalty for dissonant chords
		}

		// Randomness bias
		std::uniform_int_distribution bias(1, 3);
		if (chord.type.quality == "major" || chord.type.quality == "minor")
			score += bias(gen);
		else if (chord.type.quality == "major add9" || chord.type.quality == "minor add9")
			score -= bias(gen);

		scoredChords.emplace_back(chord, score);
	}

	// Sort chords by descending score
	std::ranges::sort(scoredChords, [](const auto& a, const auto& b) {
	    return a.second > b.second;
	});

	return scoredChords;
}


inline Chord getChord(const std::vector<Note>& segment, const Scale scale) {
	// Score the diatonic chords based on the melody segment
	const auto scored = scoreChords(segment, scale);

	if (scored.empty() || scored[0].second == 0) {
		std::cout << "No diatonic chord fits this segment well." << std::endl;
		return {};
	}

	// Print the best-matching chord
	std::cout << "Chord for this measure: "
		<< scored[0].first.name
	<< std::endl;

	return scored[0].first;
}
