-- logic.lua

-- Called once at the start of the execution
function on_start()
    -- Preload all MIDI files defined in rules
    for trigger, rule in pairs(rules) do
        if rule.theme then
            if rule.instrument then
                music.preload_midi(rule.theme)
            else
                print("[logic.lua] Warning: No instrument defined for MIDI theme '" .. rule.theme .. "' for trigger '" .. trigger .. "'. Skipping MIDI rule.")
            end
        end
    end
end


-- Track which trigger types were active in the previous update
local previous_triggers = {}

-- Called every game update
function on_update(game_state)
    if not game_state then return end
    if not game_state.playerHealth or not game_state.enemies or not game_state.environment then return end


    -- Build a set of current trigger types
    local current_triggers = {}

    for _, enemy in ipairs(game_state.enemies) do
        -- Add new enemy or increase number of same enemies
        if not enemy then return end
        current_triggers[enemy.type] = (current_triggers[enemy.type] or 0) + 1
    end

    -- Set current environment
    current_triggers[game_state.environment] = true or {}


    -- === MIDI REMOVAL (for triggers that are no longer active) ===
    for trigger_type, _ in pairs(previous_triggers) do
        if not current_triggers[trigger_type] then
            local rule = rules[trigger_type]
            if rule and rule.theme then
                music.remove_midi(rule.theme)
            end
        end
    end

    -- === MIDI ADDITION (for newly joined triggers) ===
    for trigger_type, _ in pairs(current_triggers) do
        if not previous_triggers[trigger_type] then
            local rule = rules[trigger_type]
            if rule and rule.theme and rule.instrument then
                music.add_midi(rule.theme, rule.instrument)
            elseif rule and rule.theme and not rule.instrument then
                print("[logic.lua] Warning: No instrument defined for MIDI theme '" .. rule.theme .. "' for trigger '" .. trigger_type .. "'")
            end
        end
    end


    -- Apply MIDI and layered effects for current trigger types
    apply_effects(game_state)


    -- Update previous state
    previous_triggers = current_triggers
end


-- Apply MIDI and rules for active trigger types
function apply_effects(game_state)
    local player_health = game_state.playerHealth
    local enemies       = game_state.enemies
    local environment   = game_state.environment

    local closest_enemy_rules = {}
    local closest_dist = {}

    -- Gather closest enemy rule per effect
    for _, enemy in ipairs(enemies) do
        local rule = rules[enemy.type]
        if rule then
            for k, v in pairs(rule) do
                -- Track the closest enemy with a rule for each effect
                if not closest_dist[k] or enemy.distance < closest_dist[k] then
                    closest_dist[k] = enemy.distance
                    closest_enemy_rules[k] = v
                end
            end
        end
    end

    -- Fall back to environment rule where no enemy overrides
    local env_rule = rules[environment] or {}

    local final_fx = {
        intensity        = closest_enemy_rules.intensity        or env_rule.intensity        or 0.0,
        scale            = closest_enemy_rules.scale            or env_rule.scale            or "Ionian",
        tempo_multiplier = closest_enemy_rules.tempo_multiplier or env_rule.tempo_multiplier or 1.0,
        lead_style       = closest_enemy_rules.lead_style       or env_rule.lead_style       or "Sustain",
        lead_layers      = closest_enemy_rules.lead_layers      or env_rule.lead_layers      or 1,
        chord_layers     = closest_enemy_rules.chord_layers     or env_rule.chord_layers     or 1,
        bass_style       = closest_enemy_rules.bass_style       or env_rule.bass_style       or "Sustain",
        drum_pattern     = closest_enemy_rules.drum_pattern     or env_rule.drum_pattern     or "None",
    }

    -- Apply effects
    music.set_scale(final_fx.scale)
    music.set_tempo_multiplier(final_fx.tempo_multiplier)
    music.set_lead_style(final_fx.lead_style)
    music.set_lead_layers(final_fx.lead_layers)
    music.set_chord_layers(final_fx.chord_layers)
    music.set_bass_style(final_fx.bass_style)
    music.set_drum_pattern(final_fx.drum_pattern)

    -- Adjust intensity based on player health
    local intensity = 1.0 - player_health
    music.set_intensity(final_fx.intensity + intensity)
end


function print_rule(rule)
    for k, v in pairs(rule) do
        print("  " .. k .. " = " .. tostring(v))
    end
end
