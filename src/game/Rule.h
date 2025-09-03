#pragma once

#include <string>
#include <optional>


// Rule data per mob
struct Rule {
	// Theme group
	bool addTheme = false;
	std::string theme;   // file name from /input
	int themeIdx = -1;   // index into midiFiles
	int instrument = 0;  // 0..127

	// Optional rule parameters
	std::optional<float> intensity;          // [0..1]
	std::optional<std::string> scale;        // one of kScales
	std::optional<float> tempo_multiplier;   // >= 0
	std::optional<std::string> lead_style;   // one of kLeadStyles
	std::optional<int> lead_layers;          // >= 1
	std::optional<int> chord_layers;         // >= 1
	std::optional<std::string> bass_style;   // one of kBassStyles
	std::optional<std::string> drum_pattern; // one of kDrumPatterns
};


enum class TriggerType {
	Mob = 0,
	Environment = 1,
	Tag = 2
};

inline const char* ToString(const TriggerType k) {
	switch (k) {
		case TriggerType::Mob:         return "Mob";
		case TriggerType::Environment: return "Env";
		case TriggerType::Tag:         return "Tag";
		default:                       return "ERR";
	}
}

// Key to store a rule and disambiguate identical names from different lists
struct TriggerKey {
	TriggerType type{};
	std::string name;

	bool operator==(const TriggerKey& o) const noexcept {
		return type == o.type && name == o.name;
	}
};

// Hash so we can use RuleKey in unordered_map
struct TriggerKeyHash {
	size_t operator()(const TriggerKey& k) const noexcept {
		const size_t h1 = std::hash<int>{}(static_cast<int>(k.type));
		const size_t h2 = std::hash<std::string>{}(k.name);
		return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
	}
};
