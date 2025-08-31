#pragma once

#include <functional>

#include "../util/Timing.h"
#include "Instrument.h"


enum class DrumPattern {
	NONE, CALM, STEALTH, TENSE, COMBAT, BOSS
};

using BeatOffset = double;  // e.g., 0.0 = downbeat 1, 0.5 = 8th note after downbeat, 1.75 = 16th before beat 3
using Pattern = std::unordered_map<Note, std::vector<BeatOffset>>;


/** Returns a pattern map using the provided note scheduler */
inline std::unordered_map<DrumPattern, Pattern> getDrumPatterns(const TimeSignatureInfo& tsInfo) {
	using namespace std;
	using namespace Instrument;

	if (tsInfo.num == 4 && tsInfo.denom == 4)
		return {
			{ DrumPattern::NONE, {
			}},

			{ DrumPattern::CALM, {
				{BASS_DRUM_1, { 0.0 }},
				{CLOSED_HAT,  { 0.0, 1.0, 2.0, 3.0 }}
			}},

			{ DrumPattern::STEALTH, {
				{BASS_DRUM_1, { 0.0 }},
				{PEDAL_HAT,   { 1.0, 1.67, 3.0, 3.67 }},
				{OPEN_HAT,    { 0.0, 2.0 }}
			}},

			{ DrumPattern::TENSE, {
				{BASS_DRUM_1, { 0.0, 1.5, 2.0 }},
				{CLOSED_HAT,  { 0.0, 1.0, 2.0, 3.0 }}
			}},

			{ DrumPattern::COMBAT, {
				{BASS_DRUM_1, { 0.0, 1.5, 2.5 }},
				{SNARE_1,     { 1.0, 3.0 }},
				{OPEN_HAT,    { 0.0, 1.0, 2.0, 3.0 }},
				{SPLASH_CYM,  { 0.0, 1.0, 2.0, 3.0 }}
			}},

			{ DrumPattern::BOSS, {
				{BASS_DRUM_1, { 0.0, 0.75, 1.25, 2.0, 2.75, 3.25 }},
				{SNARE_1,     { 0.5, 1.5, 2.5, 3.5 }},
				{OPEN_HAT,    { 0.0, 1.0, 2.0, 3.0 }},
				{SPLASH_CYM,  { 0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5 }}
			}}
		};

	return {};
}


inline std::string getDrumPatternName(const DrumPattern drumPattern) {
	switch (drumPattern) {
		case DrumPattern::NONE:    return "None";
		case DrumPattern::CALM:    return "Calm";
		case DrumPattern::STEALTH: return "Stealth";
		case DrumPattern::TENSE:   return "Tense";
		case DrumPattern::COMBAT:  return "Combat";
		case DrumPattern::BOSS:	   return "Boss";
	}

	throw std::invalid_argument("Invalid drum pattern");
}

inline DrumPattern getDrumPattern(const std::string& drumPattern) {
	if (drumPattern == "None")    return DrumPattern::NONE;
	if (drumPattern == "Calm")    return DrumPattern::CALM;
	if (drumPattern == "Stealth") return DrumPattern::STEALTH;
	if (drumPattern == "Tense")   return DrumPattern::TENSE;
	if (drumPattern == "Combat")  return DrumPattern::COMBAT;
	if (drumPattern == "Boss")    return DrumPattern::BOSS;

	throw std::invalid_argument("Invalid drum pattern: " + drumPattern);
}