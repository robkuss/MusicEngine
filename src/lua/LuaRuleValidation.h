#pragma once

#include "ValidLiterals.h"

#include <string>
#include <sol/sol.hpp>

struct Rule {
    bool hasTheme = false;
    bool hasInstrument = false;
};

static bool fileExists(const std::string& relPath) {
    namespace fs = std::filesystem;
    const fs::path p = fs::current_path() / "input" / relPath;
    return exists(p) && is_regular_file(p);
}

inline void parseAndValidateRule(const std::string& trigger, sol::table& cfg) {
    using namespace sol;
	using namespace std;

    Rule r;

    // Catch unknown keys
    cfg.for_each([&](const object& key, const object&) {
        if (key.get_type() == type::string) {
            const auto keyStr = key.as<string>();
            if (!inAllowedKeys(keyStr)) {
                cerr << "[Lua] Trigger \"" << trigger << "\": unknown parameter \"" << keyStr << "\" ignored\n";
            }
        } else {
            cerr << "[Lua] Trigger \"" << trigger << "\": non-string key ignored\n";
        }
    });


    bool themeKeyPresent = false;
    bool instrumentKeyPresent = false;

    // theme
    if (const auto v = cfg.get<object>("theme"); v.valid()) {
        themeKeyPresent = true;
        if (v.get_type() == type::string) {
            const auto theme = v.as<std::string>();
            r.hasTheme = true;
            if (!fileExists(theme)) {
                cerr << "[Lua] Trigger \"" << trigger << "\": theme file not found: " << theme << " -> rule disabled\n";
                r.hasTheme = false;
            }
        } else {
            cerr << "[Lua] Trigger \"" << trigger << "\": theme has wrong type -> ignored\n";
            r.hasTheme = false;
        }
    }

    // instrument
    if (const auto v = cfg.get<object>("instrument"); v.valid()) {
        instrumentKeyPresent = true;
        if (v.get_type() == type::number) {
            const int inst = v.as<int>();
            r.hasInstrument = true;
            if (inst < 0 || inst > 127) {
                cerr << "[Lua] Trigger \"" << trigger << "\": instrument out of range (0..127): "
                     << inst << " -> ignoring instrument\n";
                r.hasInstrument = false;
            }
        } else {
            cerr << "[Lua] Trigger \"" << trigger << "\": instrument has wrong type -> ignored\n";
            r.hasInstrument = false;
        }
    }

    // Consistency
    if (r.hasTheme && !r.hasInstrument) {
        if (!instrumentKeyPresent)
             cerr << "[Lua] Trigger \"" << trigger << "\": theme set but instrument missing -> MIDI rule disabled\n";
        else cerr << "[Lua] Trigger \"" << trigger << "\": theme set but instrument invalid -> MIDI rule disabled\n";
        r.hasTheme = false;
    }
    if (r.hasInstrument && !r.hasTheme) {
        if (!themeKeyPresent)
        	 cerr << "[Lua] Trigger \"" << trigger << "\": instrument set but theme missing -> MIDI rule disabled\n";
        else cerr << "[Lua] Trigger \"" << trigger << "\": instrument set but theme invalid -> MIDI rule disabled\n";
        r.hasInstrument = false;
    }

    // Additional logs: Key exists, counterpart missing completely
    if (themeKeyPresent && !instrumentKeyPresent)
        cerr << "[Lua] Trigger \"" << trigger << "\": theme key present but instrument key missing -> MIDI rule disabled\n";
    if (instrumentKeyPresent && !themeKeyPresent)
        cerr << "[Lua] Trigger \"" << trigger << "\": instrument key present but theme key missing -> MIDI rule disabled\n";

    // Only write back if key originally existed
    if (themeKeyPresent && !r.hasTheme)			  cfg["theme"]		= nil;
    if (instrumentKeyPresent && !r.hasInstrument) cfg["instrument"] = nil;


    // intensity (0..1)
    if (const auto v = cfg.get<object>("intensity"); v.valid()) {
        if (v.get_type() == type::number) {
            double x = v.as<double>();
            if (x < 0.0 || x > 1.0) {
                cerr << "[Lua] Trigger \"" << trigger << "\": intensity out of range (0..1): " << x << " -> clamped\n";
                x = clamp(x, 0.0, 1.0);
            }
            cfg["intensity"] = x;
        } else {
            // << fix: vorhandener Key mit falschem Typ -> Default setzen und zurückschreiben
            cerr << "[Lua] Trigger \"" << trigger << "\": intensity has wrong type -> default 0.0\n";
            cfg["intensity"] = 0.0;
        }
    }

    // scale (Whitelist)
    if (const auto v = cfg.get<object>("scale"); v.valid()) {
        if (v.get_type() == type::string) {
			auto s = v.as<string>();
            if (!inScales(s)) {
                cerr << "[Lua] Trigger \"" << trigger << "\": invalid scale \"" << s << "\" -> default \"Ionian\"\n";
                s = "Ionian";
            }
            cfg["scale"] = s;
        } else {
            cerr << "[Lua] Trigger \"" << trigger << "\": scale has wrong type -> default \"Ionian\"\n";
            cfg["scale"] = string("Ionian");
        }
    }

    // tempo_multiplier (>= 0.0)
    if (const auto v = cfg.get<object>("tempo_multiplier"); v.valid()) {
        if (v.get_type() == type::number) {
            double tm = v.as<double>();
            if (tm < 0.0) {
                cerr << "[Lua] Trigger \"" << trigger << "\": tempo_multiplier < 0.0 -> default 1.0\n";
                tm = 1.0;
            }
            cfg["tempo_multiplier"] = tm;
        } else {
            cerr << "[Lua] Trigger \"" << trigger << "\": tempo_multiplier has wrong type -> default 1.0\n";
            cfg["tempo_multiplier"] = 1.0;
        }
    }

    // lead_style (Whitelist)
    if (const auto v = cfg.get<object>("lead_style"); v.valid()) {
        if (v.get_type() == type::string) {
            auto ls = v.as<string>();
            if (!inLead(ls)) {
                cerr << "[Lua] Trigger \"" << trigger << "\": invalid lead_style -> default \"Sustain\"\n";
                ls = "Sustain";
            }
            cfg["lead_style"] = ls;
        } else {
            cerr << "[Lua] Trigger \"" << trigger << "\": lead_style has wrong type -> default \"Sustain\"\n";
            cfg["lead_style"] = string("Sustain");
        }
    }

    // lead_layers (>=1)
    if (const auto v = cfg.get<object>("lead_layers"); v.valid()) {
        if (v.get_type() == type::number) {
            int n = v.as<int>();
            if (n < 1) {
                cerr << "[Lua] Trigger \"" << trigger << "\": lead_layers < 1 -> set 1\n";
                n = 1;
            }
            cfg["lead_layers"] = n;
        } else {
            cerr << "[Lua] Trigger \"" << trigger << "\": lead_layers has wrong type -> set 1\n";
            cfg["lead_layers"] = 1;
        }
    }

    // chord_layers (>=1)
    if (const auto v = cfg.get<object>("chord_layers"); v.valid()) {
        if (v.get_type() == type::number) {
            int n = v.as<int>();
            if (n < 1) {
                cerr << "[Lua] Trigger \"" << trigger << "\": chord_layers < 1 -> set 1\n";
                n = 1;
            }
            cfg["chord_layers"] = n;
        } else {
            cerr << "[Lua] Trigger \"" << trigger << "\": chord_layers has wrong type -> set 1\n";
            cfg["chord_layers"] = 1;
        }
    }

    // bass_style (Whitelist)
    if (const auto v = cfg.get<object>("bass_style"); v.valid()) {
        if (v.get_type() == type::string) {
            auto bs = v.as<string>();
            if (!inBass(bs)) {
                cerr << "[Lua] Trigger \"" << trigger << "\": invalid bass_style -> default \"Sustain\"\n";
                bs = "Sustain";
            }
            cfg["bass_style"] = bs;
        } else {
            cerr << "[Lua] Trigger \"" << trigger << "\": bass_style has wrong type -> default \"Sustain\"\n";
            cfg["bass_style"] = string("Sustain");
        }
    }

    // drum_pattern (Whitelist)
    if (const auto v = cfg.get<object>("drum_pattern"); v.valid()) {
        if (v.get_type() == type::string) {
            auto dp = v.as<string>();
            if (!inDrum(dp)) {
                cerr << "[Lua] Trigger \"" << trigger << "\": invalid drum_pattern -> \"None\"\n";
                dp = "None";
            }
            cfg["drum_pattern"] = dp;
        } else {
            cerr << "[Lua] Trigger \"" << trigger << "\": drum_pattern has wrong type -> \"None\"\n";
            cfg["drum_pattern"] = string("None");
        }
    }
}
