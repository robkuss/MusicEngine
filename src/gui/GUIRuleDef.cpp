#include "GUI.h"

#include <ranges>

#include "imgui.h"

#include "game/GameData.h"
#include "lua/ValidLiterals.h"

using namespace std;


void GUI::ruleDef() {
    ImGui::Text("Game:");
    const GameData games[] = {Minecraft};
    static int gameIdx = 0;
    ImGui::Combo("##GameCombo", &gameIdx, games->name, IM_ARRAYSIZE(games));
    selectedGame = games[gameIdx];

	ImGui::Dummy(ImVec2(0, 8));
    ImGui::Separator();
	ImGui::Dummy(ImVec2(0, 8));


    ImGui::Text("Main melody source:");

    // Main melody
	int mainMidiIdx = -1;
	if (!midiFiles.empty()) {
		vector<const char*> items;
		for (auto& f : midiFiles) {
			items.push_back(f.c_str());
		}
		for (int i = 0; i < static_cast<int>(midiFiles.size()); ++i) {
			if (midiFiles[i] == selectedMidi ||
				filesystem::path(midiFiles[i]).filename() ==
				filesystem::path(selectedMidi).filename()
			) {
				mainMidiIdx = i;
				break;
			}
		}
		if (mainMidiIdx < 0) mainMidiIdx = 0;

		if (ImGui::Combo("##MainMelody", &mainMidiIdx, items.data(), static_cast<int>(items.size())))
			selectedMidi = midiFiles[mainMidiIdx];
		else selectedMidi = midiFiles[mainMidiIdx];
	} else {
		ImGui::TextDisabled("No MIDI files found");
	}

    ImGui::Separator();

	ImGui::Text("Markov order:");

	ImGui::Checkbox("Auto Markov", &autoMarkov);
	ImGui::BeginDisabled(autoMarkov);
	ImGui::InputInt("Order", &markovOrder, 1, 5, ImGuiInputTextFlags_CharsDecimal);
	if (markovOrder < 1) markovOrder = 1;
	ImGui::EndDisabled();


	// Rule definitions
	ImGui::Dummy(ImVec2(0, 8));
	ImGui::Separator();
	ImGui::Dummy(ImVec2(0, 24));

	ImGui::Text("Rule definitions");

	// Top bar with Add Rule
	if (ImGui::Button("Add Rule")) {
	    ImGui::OpenPopup("Add Rule");
	}
	ImGui::SameLine();
	if (ImGui::Button("Refresh MIDI list")) {
	    midiFiles = scanMidiFiles();
	}

	if (ImGui::IsPopupOpen("Add Rule")) {
	    const ImGuiViewport* vp = ImGui::GetMainViewport();
	    ImVec2 center(vp->Pos.x + vp->Size.x * 0.5f, vp->Pos.y + vp->Size.y * 0.5f);
	    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	}

	const bool opened = ImGui::BeginPopupModal(
	    "Add Rule",
	    nullptr,
	    ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove
	);

	if (opened) {
	    static int typeIdx = 0;  // 0 = Mob, 1 = Environment, 2 = Tag
	    const char* types[] = {"Mob", "Environment", "Tag"};
	    ImGui::Text("Trigger type:");
	    ImGui::Combo("##TriggerType", &typeIdx, types, IM_ARRAYSIZE(types));
	    ImGui::Separator();

	    // Build the source list for the chosen kind
	    const vector<string> *src = nullptr;
		auto chosenType = TriggerType::Mob;
		switch (typeIdx) {
			case 0:
				src = &selectedGame.mobsList;
				chosenType = TriggerType::Mob;
				break;
			case 1:
				src = &selectedGame.envList;
				chosenType = TriggerType::Environment;
				break;
			default:
				src = &selectedGame.tagsList;
				chosenType = TriggerType::Tag;
		}

	    // Compute "available" = items in src minus names already present *for this type*
	    vector<string> available;
	    if (src) {
	        available.reserve(src->size());
	        for (const auto& n : *src) {
	            TriggerKey k{chosenType, n};
	            if (!rules.contains(k)) available.push_back(n);
	        }
	    }

	    static int addIdx = 0;
	    if (available.empty()) {
	        ImGui::TextDisabled("No more %s entries available to add.", types[typeIdx]);
	        if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
	    } else {
	    	vector<string> displayItems;
	    	vector<const char*> items;
	    	displayItems.reserve(available.size());
	    	items.reserve(available.size());
	    	for (auto& s : available) {
	    		displayItems.push_back(uncanonicalize(s));
	    		items.push_back(displayItems.back().c_str());
	    	}

	    	ImGui::Text("Select %s:", types[typeIdx]);
	    	ImGui::Combo("##AddItemCombo", &addIdx, items.data(), static_cast<int>(items.size()));

	        ImGui::Separator();
	        if (ImGui::Button("Add##Confirm")) {
	            TriggerKey key{chosenType, available[addIdx]};
	            rules[key] = Rule{};                // default rule
	            ruleOrder.push_back(key);           // keep stable order
	            selectedRuleIdx = static_cast<int>(ruleOrder.size()) - 1;
	            addIdx = 0;                         // reset for next time
	            ImGui::CloseCurrentPopup();
	        }
	        ImGui::SameLine();
	        if (ImGui::Button("Cancel##CancelAdd")) {
	            ImGui::CloseCurrentPopup();
	        }
	    }

	    ImGui::EndPopup();
	}

	sort(ruleOrder.begin(), ruleOrder.end(),
	[](const TriggerKey& a, const TriggerKey& b){
		if (a.type != b.type)
			return static_cast<int>(a.type) < static_cast<int>(b.type);
		return a.name < b.name;
	});


	// Left: rule selection
	const float availW = ImGui::GetContentRegionAvail().x;
	const float leftW  = max(200.0f, availW * 0.25f);

	ImGui::BeginChild("##RuleList", ImVec2(leftW, 480), true);
	{
    	if (ruleOrder.empty()) {
    		ImGui::TextDisabled("No rules yet.\nClick 'Add Rule' to create one.");
    	} else {
    		for (int i = 0; i < static_cast<int>(ruleOrder.size()); ++i) {
    			const TriggerKey& k = ruleOrder[i];
    			const bool selected = selectedRuleIdx == i;

    			// Label like: "[Mob] Creeper"
    			string label = "[" + string(ToString(k.type)) + "] " + uncanonicalize(k.name);

    			ImVec2 fullRow(ImGui::GetContentRegionAvail().x, 0);
    			if (ImGui::Selectable(label.c_str(), selected, 0, fullRow)) {
    				selectedRuleIdx = i;
    			}

    			// Context menu to remove
    			string ctxId = "##ctx_" + to_string(static_cast<int>(k.type)) + "_" + k.name;
    			if (ImGui::BeginPopupContextItem(ctxId.c_str())) {
    				if (ImGui::MenuItem("Remove rule")) {
    					rules.erase(k);
    					ruleOrder.erase(ruleOrder.begin() + i);
    					if (selectedRuleIdx >= static_cast<int>(ruleOrder.size()))
    						selectedRuleIdx = static_cast<int>(ruleOrder.size()) - 1;
    					ImGui::EndPopup();
    					break;
    				}
    				ImGui::EndPopup();
    			}
    		}
    	}
	}
	ImGui::EndChild();

	ImGui::SameLine();

	// Right: rule configurations
	ImGui::BeginChild("##RuleEditor", ImVec2(0, 480), true);
	{
    	if (selectedRuleIdx < 0 || selectedRuleIdx >= static_cast<int>(ruleOrder.size())) {
    		ImGui::TextDisabled("Select a rule from the left to edit.");
    	} else {
    		const TriggerKey& key = ruleOrder[selectedRuleIdx];

    		auto &[
				addTheme,
				theme,
				themeIdx,
				instrument,
				intensity,
				scale,
				tempo_multiplier,
				lead_style,
				lead_layers,
				chord_layers,
				bass_style,
				drum_pattern
			] = rules[key];

    		ImGui::Text("%s", uncanonicalize(key.name).c_str());
    		ImGui::Separator();

	        // --- Theme (required pair: theme + instrument) ---
	        ImGui::Checkbox("Add Theme", &addTheme);
	        if (addTheme) {
	            if (!midiFiles.empty()) {
	                if (themeIdx < 0) themeIdx = 0;

	                vector<const char*> items;
	                items.reserve(midiFiles.size());
	                for (auto& f : midiFiles) items.push_back(f.c_str());

	                ImGui::Combo("Theme (MIDI from /input)##ThemeMidi", &themeIdx, items.data(), static_cast<int>(items.size()));
	                if (themeIdx >= 0) theme = midiFiles[themeIdx];
	            } else {
	                ImGui::TextDisabled("No MIDI files found in /input");
	                theme.clear();
	                themeIdx = -1;
	            }

	            ImGui::InputInt("Instrument (0–127)##ThemeInstrument", &instrument, 1, 10, ImGuiInputTextFlags_CharsDecimal);
	            if (instrument < 0)   instrument = 0;
	            if (instrument > 127) instrument = 127;

	            if (theme.empty()) {
	                ImGui::TextColored(ImVec4(1,0.6f,0,1), "Select a MIDI file for the theme.");
	            }
	        }

	    	ImGui::Spacing();
	        ImGui::SeparatorText("Parameters");
	    	ImGui::Spacing();

	    	// Button to open popup
			if (ImGui::Button("Add Parameter")) {
			    ImGui::OpenPopup("AddParamPopup");
			}

			if (ImGui::BeginPopup("AddParamPopup")) {
			    // Build list of available params
			    vector<string> availableParams;
			    if (!intensity)        availableParams.emplace_back("intensity");
			    if (!scale)            availableParams.emplace_back("scale");
			    if (!tempo_multiplier) availableParams.emplace_back("tempo_multiplier");
			    if (!lead_style)       availableParams.emplace_back("lead_style");
			    if (!lead_layers)      availableParams.emplace_back("lead_layers");
			    if (!chord_layers)     availableParams.emplace_back("chord_layers");
			    if (!bass_style)       availableParams.emplace_back("bass_style");
			    if (!drum_pattern)     availableParams.emplace_back("drum_pattern");

			    static int addParamIdx = 0;
			    if (!availableParams.empty()) {
			        vector<const char*> items;
			        items.reserve(availableParams.size());
			        for (auto& s : availableParams) {
				        items.push_back(s.c_str());
			        }

			        ImGui::Combo("Select parameter", &addParamIdx, items.data(), static_cast<int>(items.size()));

			        if (ImGui::Button("Add##ConfirmParam")) {
			            const string& sel = availableParams[addParamIdx];
			            // Initialize with sensible defaults
			            if		(sel == "intensity")        intensity		= 0.0f;
			            else if (sel == "scale")            scale			 = "Ionian";
			            else if (sel == "tempo_multiplier") tempo_multiplier = 1.0f;
			            else if (sel == "lead_style")       lead_style		 = "Sustain";
			            else if (sel == "lead_layers")      lead_layers		= 1;
			            else if (sel == "chord_layers")     chord_layers	= 1;
			            else if (sel == "bass_style")       bass_style		 = "Sustain";
			            else if (sel == "drum_pattern")     drum_pattern	 = "None";

			            addParamIdx = 0;  // reset
			            ImGui::CloseCurrentPopup();
			        }
			        ImGui::SameLine();
			        if (ImGui::Button("Cancel##CancelParam")) {
			            ImGui::CloseCurrentPopup();
			        }
			    } else {
			        ImGui::TextDisabled("All parameters already added.");
			        if (ImGui::Button("Close")) {
			            ImGui::CloseCurrentPopup();
			        }
			    }

			    ImGui::EndPopup();
			}

	        ImGui::Separator();

	        // --- Render present parameters, each with a Remove button ---
	        // intensity
	        if (intensity) {
	            float v = *intensity;
	            ImGui::PushID("intensity");
	            if (ImGui::SliderFloat("intensity", &v, 0.0f, 1.0f))
	            	intensity = v;
	            ImGui::SameLine();
	            if (ImGui::SmallButton("Remove"))
	            	intensity.reset();
	            ImGui::PopID();
	        }

	        // scale (enum from kScales)
	        if (scale) {
	            // Build a sorted list for stable order
	            vector list(kScales.begin(), kScales.end());
	            sort(list.begin(), list.end());
	            int idx = 0;
	            for (int i = 0; i < static_cast<int>(list.size()); ++i) {
		            if (list[i] == *scale) {
		            	idx = i;
		            	break;
		            }
	            }
	            vector<const char*> items;
	        	items.reserve(list.size());
	            for (auto& s : list) {
		            items.push_back(s.c_str());
	            }

	            ImGui::PushID("scale");
	            if (ImGui::Combo("scale", &idx, items.data(), static_cast<int>(items.size())))
	            	scale = list[idx];
	            ImGui::SameLine();
	            if (ImGui::SmallButton("Remove"))
	            	scale.reset();
	            ImGui::PopID();
	        }

	        // tempo_multiplier
	        if (tempo_multiplier) {
	            float t = *tempo_multiplier;
	            ImGui::PushID("tempo_multiplier");
	            if (ImGui::InputFloat("tempo_multiplier", &t, 0.1f, 1.0f, "%.2f", ImGuiInputTextFlags_CharsDecimal)) {
	                if (t < 0.0f) t = 0.0f;
	                tempo_multiplier = t;
	            }
	            ImGui::SameLine();
	            if (ImGui::SmallButton("Remove"))
	            	tempo_multiplier.reset();
	            ImGui::PopID();
	        }

	        // lead_style
	        if (lead_style) {
	            vector list(kLeadStyles.begin(), kLeadStyles.end());
	            sort(list.begin(), list.end());
	            int idx = 0;
	        	for (int i = 0; i < static_cast<int>(list.size()); ++i) {
	        		if (list[i] == *lead_style) {
	        			idx = i;
	        			break;
	        		}
	        	}
	            vector<const char*> items;
	        	for (auto& s : list) {
	        		items.push_back(s.c_str());
	        	}

	            ImGui::PushID("lead_style");
	            if (ImGui::Combo("lead_style", &idx, items.data(), static_cast<int>(items.size())))
	            	lead_style = list[idx];
	            ImGui::SameLine();
	            if (ImGui::SmallButton("Remove"))
	            	lead_style.reset();
	            ImGui::PopID();
	        }

	        // lead_layers
	        if (lead_layers) {
	            int n = *lead_layers;
	            ImGui::PushID("lead_layers");
	            if (ImGui::InputInt("lead_layers", &n)) {
	                if (n < 1) n = 1;
	                lead_layers = n;
	            }
	            ImGui::SameLine();
	            if (ImGui::SmallButton("Remove"))
	            	lead_layers.reset();
	            ImGui::PopID();
	        }

	        // chord_layers
	        if (chord_layers) {
	            int n = *chord_layers;
	            ImGui::PushID("chord_layers");
	            if (ImGui::InputInt("chord_layers", &n)) {
	                if (n < 1) n = 1;
	                chord_layers = n;
	            }
	            ImGui::SameLine();
	            if (ImGui::SmallButton("Remove"))
	            	chord_layers.reset();
	            ImGui::PopID();
	        }

	        // bass_style
	        if (bass_style) {
	            vector list(kBassStyles.begin(), kBassStyles.end());
	            sort(list.begin(), list.end());
	            int idx = 0;
	        	for (int i = 0; i < static_cast<int>(list.size()); ++i) {
	        		if (list[i] == *bass_style) {
	        			idx = i;
	        			break;
	        		}
	        	}
	            vector<const char*> items;
	        	for (auto& s : list) {
	        		items.push_back(s.c_str());
	        	}
	            ImGui::PushID("bass_style");
	            if (ImGui::Combo("bass_style", &idx, items.data(), static_cast<int>(items.size())))
	            	bass_style = list[idx];
	            ImGui::SameLine();
	            if (ImGui::SmallButton("Remove"))
	            	bass_style.reset();
	            ImGui::PopID();
	        }

	        // drum_pattern
	        if (drum_pattern) {
	            vector list(kDrumPatterns.begin(), kDrumPatterns.end());
	            sort(list.begin(), list.end());
	            int idx = 0;
	        	for (int i = 0; i < static_cast<int>(list.size()); ++i) {
	        		if (list[i] == *drum_pattern) {
	        			idx = i;
	        			break;
	        		}
	        	}
	            vector<const char*> items;
	        	for (auto& s : list) {
	        		items.push_back(s.c_str());
	        	}

	            ImGui::PushID("drum_pattern");
	            if (ImGui::Combo("drum_pattern", &idx, items.data(), static_cast<int>(items.size())))
	            	drum_pattern = list[idx];
	            ImGui::SameLine();
	            if (ImGui::SmallButton("Remove"))
	            	drum_pattern.reset();
	            ImGui::PopID();
	        }

    		ImGui::Separator();
    		if (ImGui::Button("Remove this rule")) {
    			rules.erase(key);
    			ruleOrder.erase(ruleOrder.begin() + selectedRuleIdx);
    			if (selectedRuleIdx >= static_cast<int>(ruleOrder.size())) {
    				selectedRuleIdx = static_cast<int>(ruleOrder.size()) - 1;
    			}
    		}
    	}
	}
	ImGui::EndChild();


	// --- Export ---
	ImGui::Dummy(ImVec2(0, 18));
	ImGui::SeparatorText("Export");

	ImGui::InputText("Output file", outPath, IM_ARRAYSIZE(outPath));
	ImGui::SameLine();

	if (ImGui::Button("Export to Lua")) {
		exportLua(outPath);
	}

	if (ImGui::BeginPopupModal(
		"ExportResult",
		nullptr,
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoMove
	)) {
		static string lastMsg;
		ImGui::TextWrapped("%s", lastMsg.c_str());
		ImGui::Dummy(ImVec2(0, 8));
		if (ImGui::Button("OK", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}


	// --- Start Music ---
	ImGui::Dummy(ImVec2(0, 18));

	// Determine if we can start
	bool hasMainMelody = !selectedMidi.empty();
	bool hasAnyRule    = !rules.empty();

	bool allThemesValid = true;
	for (const auto &r: rules | views::values) {
		if (r.addTheme) {
			if (r.theme.empty() || r.instrument < 0 || r.instrument > 127) {
				allThemesValid = false;
				break;
			}
		}
	}

	const bool canStart = hasMainMelody && hasAnyRule && allThemesValid;

	constexpr auto blue       = ImVec4(0.20f, 0.55f, 0.95f, 1.0f);
	constexpr auto blueHover  = ImVec4(0.26f, 0.62f, 1.00f, 1.0f);
	constexpr auto blueActive = ImVec4(0.16f, 0.49f, 0.90f, 1.0f);

	ImGui::PushStyleColor(ImGuiCol_Button,        blue);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, blueHover);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive,  blueActive);

	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);

	if (!canStart) ImGui::BeginDisabled(true);

	ImVec2 btnSize(240.0f, 48.0f);
	float cursorX = (availW - btnSize.x) * 0.5f;
	if (cursorX > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + cursorX);

	if (ImGui::Button("Start Music!", btnSize)) {
		// Persist current settings so MusicMaker can read them
		exportLua(outPath);

		// Make sure any previous run is shut down
		stopMusicIfRunning();

		// Start fresh MusicMaker in the background
		music = make_unique<MusicMaker>();
		musicRunning.store(true, memory_order_release);
		musicPaused.store(false, memory_order_release);

		musicThread = thread([this]{
			music->start();                  // blocking in this worker thread
			musicRunning.store(false, memory_order_release);
			musicPaused.store(false, memory_order_release);
		});

		// Go to MusicGen screen
		guiState = GUIState::MusicGen;
	}

	if (!canStart) {
		ImGui::EndDisabled();
		if (!hasMainMelody)  ImGui::TextColored(ImVec4(1, 0.6f, 0, 1), "Select a main melody source.");
		if (!hasAnyRule)     ImGui::TextColored(ImVec4(1, 0.6f, 0, 1), "Add at least one rule.");
		if (!allThemesValid) ImGui::TextColored(ImVec4(1, 0.6f, 0, 1), "Fix theme parameters: missing MIDI or invalid instrument (0–127).");
	}

	// Restore previous style
	ImGui::PopStyleVar(1);
	ImGui::PopStyleColor(3);
}
