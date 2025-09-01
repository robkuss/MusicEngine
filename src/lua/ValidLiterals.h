#pragma once

#include <unordered_set>
#include <string>

static const std::unordered_set<std::string> kAllowedKeys = {
	"theme",
	"instrument",
	"intensity",
	"scale",
	"tempo_multiplier",
	"lead_style",
	"lead_layers",
	"chord_layers",
	"bass_style",
	"drum_pattern"
};

static const std::unordered_set<std::string> kScales = {
	"Lydian",
	"Ionian",
	"Mixolydian",
	"Dorian",
	"Aeolian",
	"Phrygian",
	"Locrian"
};

static const std::unordered_set<std::string> kLeadStyles = {
	"Sustain",
	"Pulse"
};

static const std::unordered_set<std::string> kBassStyles = {
	"Sustain",
	"Pulse",
	"Fast"
};

static const std::unordered_set<std::string> kDrumPatterns = {
	"None",
	"Calm",
	"Stealth",
	"Tense",
	"Combat",
	"Boss"
};


// Helpers
inline bool inAllowedKeys(const std::string& s) { return kAllowedKeys.contains(s); }
inline bool inScales(const std::string& s) { return kScales.contains(s); }
inline bool inLead(const std::string& s) { return kLeadStyles.contains(s); }
inline bool inBass(const std::string& s) { return kBassStyles.contains(s); }
inline bool inDrum(const std::string& s) { return kDrumPatterns.contains(s); }
