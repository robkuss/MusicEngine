#pragma once

#include <array>
#include <util/Util.h>
#include <vector>


enum class Scale {
	LYDIAN, IONIAN, MIXOLYDIAN, DORIAN, AEOLIAN, PHRYGIAN, LOCRIAN
};


inline std::vector<int> getScaleIntervals(const Scale mode) {
	switch (mode) {
		case Scale::LYDIAN:     return {0, 2, 4, 6, 7, 9, 11};
		case Scale::IONIAN:     return {0, 2, 4, 5, 7, 9, 11};
		case Scale::MIXOLYDIAN: return {0, 2, 4, 5, 7, 9, 10};
		case Scale::DORIAN:     return {0, 2, 3, 5, 7, 9, 10};
		case Scale::AEOLIAN:    return {0, 2, 3, 5, 7, 8, 10};
		case Scale::PHRYGIAN:   return {0, 1, 3, 5, 7, 8, 10};
		case Scale::LOCRIAN:    return {0, 1, 3, 5, 6, 8, 10};
	}

	throw std::invalid_argument("Invalid mode");
}

inline std::string getScaleName(const Scale mode) {
	switch (mode) {
		case Scale::LYDIAN:     return "Lydian";
		case Scale::IONIAN:     return "Ionian (major)";
		case Scale::MIXOLYDIAN: return "Mixolydian";
		case Scale::DORIAN:     return "Dorian";
		case Scale::AEOLIAN:    return "Aeolian (minor)";
		case Scale::PHRYGIAN:   return "Phrygian";
		case Scale::LOCRIAN:    return "Locrian";
	}

	throw std::invalid_argument("Invalid mode");
}

inline Scale getScaleFromName(const std::string& mode) {
	if (mode == "Lydian")     return Scale::LYDIAN;
	if (mode == "Ionian")     return Scale::IONIAN;
	if (mode == "Mixolydian") return Scale::MIXOLYDIAN;
	if (mode == "Dorian")     return Scale::DORIAN;
	if (mode == "Aeolian")    return Scale::AEOLIAN;
	if (mode == "Phrygian")   return Scale::PHRYGIAN;
	if (mode == "Locrian")    return Scale::LOCRIAN;

	throw std::invalid_argument("Invalid mode: " + mode);
}


inline std::array<int, 7> buildScale(const int root, const Scale mode) {
	std::array<int, 7> scale{};
	const auto& intervals = getScaleIntervals(mode);
	for (size_t i = 0; i < 7; ++i) {
		scale[i] = (root + intervals[i]) % 12;
	}
	return scale;
}

inline Note changeNoteForScale(const Note note, const Note key, const Scale sourceScale, const Scale targetScale) {
	const int octave	  = note / 12;
	const int pitchClass  = note % 12;
	const int sourceClass = key % 12;

	// Get the diatonic scale for the key
	const auto sourceIntervals = getScaleIntervals(sourceScale);
	const auto targetIntervals = getScaleIntervals(targetScale);

	// Map pitch class to scale degree in source key
	int degree = -1;
	for (int i = 0; i < sourceIntervals.size(); ++i) {
		if ((sourceClass + sourceIntervals[i]) % 12 == pitchClass) {
			degree = i;
			break;
		}
	}
	if (degree == -1) return note;  // Note doesn't exist in source scale — fallback

	// Replace with equivalent degree in target scale
	const int newPitchClass = (sourceClass + targetIntervals[degree]) % 12;
	int newNote = octave * 12 + newPitchClass;

	// Wrap within valid MIDI note range [0, 127]
	while (newNote < 0)   newNote += 12;
	while (newNote > 127) newNote -= 12;

	return newNote;
}
