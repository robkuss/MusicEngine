-- rules.lua

-- Config (set before training)
local main_midi = "sus.mid"
music.preload_midi(main_midi)
music.set_main_midi(main_midi)
music.use_auto_markov(false)
music.set_markov_order(3)


-- Rules setup
--[[ Rules can be:
    - theme             (connects a MIDI theme to a trigger)
    - instrument        (connects that MIDI theme to a MIDI instrument)
    - intensity         (0.0 - 1.0, experimental)
    - scale             (any diatonic mode, e.g. "Dorian" or "Lydian")
    - tempo_multiplier  (experimental)
    - lead_style        ("Sustain" or "Pulse")
    - lead_layers       (>= 1)
    - chord_layers      (>= 1)
    - bass_style        ("Sustain", "Pulse", "Fast")
    - drum_pattern      ("None", "Calm", "Stealth", "Tense", "Combat", "Boss")
]]
rules = {}

-- Add a rule for a trigger (enemy or environment)
function add_rule(trigger, config)
    if rules[trigger] ~= nil then
        print("[Lua] Error: rule for trigger '" .. trigger .. "' is already defined -> ignored")
        return
    end
    rules[trigger] = config
end


-- Example rule definitions

-- Enemy rules

add_rule("zombie", {
    theme = "zombie.mid",
    instrument = 11,  -- Vibraphone
    lead_layers = 2,
    bass_style = "Pulse"
})

add_rule("skeleton", {
    theme = "skeleton.mid",
    instrument = 13,  -- Xylophone
    bass_style = "Pulse"
})

add_rule("wither", {
    theme = "skeleton.mid",
    instrument = 61,  -- Brass Section
    lead_layers = 3,
    chord_layers = 3,
    lead_style = "Pulse",
    bass_style = "Fast",
    drum_pattern = "Boss"
})

-- Environment rules
add_rule("plains", {
    scale = "Ionian",
    drum_pattern = "None"
})

add_rule("nether_wastes", {
    lead_layers = 2,
    scale = "Phrygian",
    drum_pattern = "Tense"
})
