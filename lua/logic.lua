-- logic.lua

-- Helpers
local function has(list, value)
    for _, v in ipairs(list) do if v == value then return true end end
    return false
end

local function first_non_nil(...)
    for i = 1, select("#", ...) do
        local v = select(i, ...)
        if v ~= nil then return v end
    end
    return nil
end


-- Called once at the start of the execution
function on_start()
    -- Preload all MIDI files defined in rules
    local preloaded = {}
    for _, rule in pairs(rules) do
        if rule.theme and rule.instrument then
            if not preloaded[rule.theme] then
                music.preload_midi(rule.theme)
                preloaded[rule.theme] = true
            end
        end
    end
end


-- Track which trigger types were active in the previous update
local previous_triggers = {}

-- Called every game update
function on_update(game_state)
    if not game_state
    or not game_state.playerHealth
    or not game_state.enemies
    or not game_state.environment then
        return
    end

    -- Build a set of current trigger types
    local current_triggers = {}

    for _, enemy in ipairs(game_state.enemies) do
        -- Add new enemy or increase number of same enemies
        if not enemy then return end
        current_triggers[enemy.type] = (current_triggers[enemy.type] or 0) + 1
    end

    -- Set current environment
    local env_type = game_state.environment
    local env_tags = {}

    if type(game_state.environmentTags) == "table" then
        for _, t in ipairs(game_state.environmentTags) do
            if type(t) == "string" and t ~= "" then
                table.insert(env_tags, t)
            end
        end
    end

    if type(env_type) == "string" and env_type ~= "" then
        current_triggers[env_type] = true
    end
    for _, tag in ipairs(env_tags) do
        current_triggers[tag] = true
    end


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
            end
        end
    end


    -- Apply MIDI and layered effects for current trigger types
    apply_effects(game_state)


    -- Update previous state (shallow copy to avoid aliasing)
    local tmp = {}
    for k, v in pairs(current_triggers) do tmp[k] = v end
    previous_triggers = current_triggers
end


-- Apply MIDI and rules for active trigger types
function apply_effects(game_state)
    local player_health = game_state.playerHealth
    local enemies       = game_state.enemies
    local env_type      = game_state.environment
    local env_tags      = {}

    if type(game_state.environmentTags) == "table" then
        for _, t in ipairs(game_state.environmentTags) do
            if type(t) == "string" and t ~= "" then table.insert(env_tags, t) end
        end
    end

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

    -- Environment rule
    local env_rule = {}

    local tag_priority = { "thunder", "snow", "rain", "night", "clear", "day" }
    for _, tag in ipairs(tag_priority) do
        if has(env_tags, tag) and rules[tag] then
            env_rule = rules[tag]
            break
        end
    end
    if next(env_rule) == nil and type(env_type) == "string" and rules[env_type] then
        env_rule = rules[env_type]
    end

    -- Unified music configuration
    local final_fx = {
        intensity        = first_non_nil(closest_enemy_rules.intensity,        env_rule.intensity,        0.0),
        scale            = first_non_nil(closest_enemy_rules.scale,            env_rule.scale,            "Ionian"),
        tempo_multiplier = first_non_nil(closest_enemy_rules.tempo_multiplier, env_rule.tempo_multiplier, 1.0),
        lead_style       = first_non_nil(closest_enemy_rules.lead_style,       env_rule.lead_style,       "Sustain"),
        lead_layers      = first_non_nil(closest_enemy_rules.lead_layers,      env_rule.lead_layers,      1),
        chord_layers     = first_non_nil(closest_enemy_rules.chord_layers,     env_rule.chord_layers,     1),
        bass_style       = first_non_nil(closest_enemy_rules.bass_style,       env_rule.bass_style,       "Sustain"),
        drum_pattern     = first_non_nil(closest_enemy_rules.drum_pattern,     env_rule.drum_pattern,     "None"),
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
    local total_intensity = (final_fx.intensity or 0.0) + intensity
    if total_intensity < 0.0 then total_intensity = 0.0 end
    if total_intensity > 1.0 then total_intensity = 1.0 end
    music.set_intensity(final_fx.intensity + intensity)
end
