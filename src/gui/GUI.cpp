#include "GUI.h"

#include <iostream>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "lua/ValidLiterals.h"

using namespace std;


// List of available mobs (game-specific, extend as needed)
static const vector<string> mobsList = {
	"Zombie", "Skeleton", "Wither"
};


void GUI::errorCallback(const int code, const char* msg) {
	cerr << "[GLFW] error " << code << ": " << msg << "\n";
}

int windowWidth, windowHeight;
void GUI::windowSizeCallback(GLFWwindow* window, const int width, const int height) {
	windowWidth = width;
	windowHeight = height;
}


GUI::GUI(
	const string& title,
	const int width,
	const int height
) : width(width), height(height), title(title) {
	windowWidth = width;
	windowHeight = height;

	midiFiles = scanMidiFiles();
}

void GUI::start() {
	glfwSetErrorCallback(errorCallback);

	if (!glfwInit()) {
		cerr << "GLFW init failed\n";
		return;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* win = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
	if (!win) {
		cerr << "Window creation failed\n";
		glfwTerminate();
		return;
	}

	glfwMakeContextCurrent(win);
	glfwSwapInterval(1);  // VSync

	glfwSetWindowSizeCallback(win, windowSizeCallback);

	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
		cerr << "Failed to initialize GLAD\n";
		return;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/arial.ttf", 20.0f);
	io.FontGlobalScale = 1.0f;

	ImGui::StyleColorsDark();

	// Bind backend
	ImGui_ImplGlfw_InitForOpenGL(win, true);
	ImGui_ImplOpenGL3_Init("#version 330");  // Shader version


	while (!glfwWindowShouldClose(win)) {
		glfwPollEvents();

		// Begin ImGui Frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		int display_w, display_h;
		glfwGetFramebufferSize(win, &display_w, &display_h);

		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(static_cast<float>(windowWidth), static_cast<float>(windowHeight)), ImGuiCond_Always);

		constexpr ImGuiWindowFlags flags =
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoCollapse;

		// UI goes here
		ImGui::Begin("Music Engine", nullptr, flags);

		switch (guiState) {
			case GUIState::TitleScreen:
				titleScreen();
				break;
			case GUIState::Project:
				project();
				break;
			default: break;
		}

		ImGui::End();

		// Prepare render
		ImGui::Render();
		int fb_w, fb_h;
		glfwGetFramebufferSize(win, &fb_w, &fb_h);
		glViewport(0, 0, fb_w, fb_h);
		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Render ImGui DrawData
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(win);
	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(win);
	glfwTerminate();
}


void GUI::titleScreen() {
	// Center the button
	constexpr ImVec2 buttonSize(200, 80);
	const ImVec2 winSize = ImGui::GetContentRegionAvail();
	ImGui::SetCursorPosX((winSize.x - buttonSize.x) * 0.5f);
	ImGui::SetCursorPosY((winSize.y - buttonSize.y) * 0.5f);

	if (ImGui::Button("New Project", buttonSize)) {
		guiState = GUIState::Project;
	}
}

void GUI::project() {
    ImGui::Text("Game:");
    const char* games[] = {"Minecraft"};
    static int gameIdx = 0;
    ImGui::Combo("##GameCombo", &gameIdx, games, IM_ARRAYSIZE(games));
    selectedGame = games[gameIdx];

	ImGui::Dummy(ImVec2(0, 8));
    ImGui::Separator();
	ImGui::Dummy(ImVec2(0, 8));

    ImGui::Text("Main melody source:");

    // Scan once (or add a Refresh button later)
    static vector<string> midiFiles = scanMidiFiles();

    // Main melody
    static int mainMidiIdx = midiFiles.empty() ? -1 : 0;
    if (!midiFiles.empty()) {
        vector<const char*> items;
        items.reserve(midiFiles.size());
        for (auto& f : midiFiles) items.push_back(f.c_str());

        ImGui::Combo("##MainMelody", &mainMidiIdx, items.data(), static_cast<int>(items.size()));
        if (mainMidiIdx >= 0) selectedMidi = midiFiles[mainMidiIdx];
    } else {
        ImGui::TextDisabled("No MIDI files found");
    }

    ImGui::Separator();

    ImGui::Text("Markov order:");

    static bool autoMarkov = true;
    ImGui::Checkbox("Auto Markov", &autoMarkov);

    static int markovOrder = 1;
    ImGui::BeginDisabled(autoMarkov);
    ImGui::InputInt("Order", &markovOrder, 1, 5, ImGuiInputTextFlags_CharsDecimal);
    if (markovOrder < 1) markovOrder = 1;
    ImGui::EndDisabled();


	// Rule definitions
	ImGui::Dummy(ImVec2(0, 8));
	ImGui::Separator();
	ImGui::Dummy(ImVec2(0, 24));

	ImGui::Text("Rule definitions");

	// Keep display order stable for added rules
	static vector<string> ruleOrder;	// names of mobs in display order
	static int selectedRuleIdx = -1;	// index in ruleOrder (not mobsList)

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
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoMove
	);

	if (opened) {
	    // Build available mobs
	    vector<string> available;
	    available.reserve(mobsList.size());
	    for (const auto& m : mobsList) {
	        if (!rules.contains(m)) available.push_back(m);
	    }

	    static int addIdx = 0;  // selection in popup
	    if (available.empty()) {
	        ImGui::TextDisabled("No more mobs available to add.");
	        if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
	    } else {
	        // Display a combo with available mobs
	        vector<const char*> items;
	        items.reserve(available.size());
	        for (auto& s : available) items.push_back(s.c_str());

	        ImGui::Text("Select trigger:");
	        ImGui::Combo("##AddMobCombo", &addIdx, items.data(), static_cast<int>(items.size()));

	        ImGui::Separator();
	        if (ImGui::Button("Add##Confirm")) {
	            const string mob = available[addIdx];
	            rules[mob] = Rule{};	   // Create default rule
	            ruleOrder.push_back(mob);  // Keep a stable order vector

	        	// Select the newly added rule
	            selectedRuleIdx = static_cast<int>(ruleOrder.size()) - 1;
	            ImGui::CloseCurrentPopup();
	        }
	        ImGui::SameLine();
	        if (ImGui::Button("Cancel##CancelAdd")) {
	            ImGui::CloseCurrentPopup();
	        }
	    }
	    ImGui::EndPopup();
	}

	// Left: mob selection
	const float availW = ImGui::GetContentRegionAvail().x;
	const float leftW  = max(200.0f, availW * 0.25f);

	ImGui::BeginChild("##RuleList", ImVec2(leftW, 480), true);
	{
    	if (ruleOrder.empty()) {
    		ImGui::TextDisabled("No rules yet.\nClick 'Add Rule' to create one.");
    	} else {
    		for (int i = 0; i < static_cast<int>(ruleOrder.size()); ++i) {
    			const string& mob = ruleOrder[i];
    			const bool selected = selectedRuleIdx == i;

    			// Visible label plus unique ID
    			string label = mob + "###rule_" + mob;

    			ImVec2 fullRow(ImGui::GetContentRegionAvail().x, 0);
    			if (ImGui::Selectable(label.c_str(), selected, 0, fullRow)) {
    				selectedRuleIdx = i;
    			}

    			// Context menu to remove
    			if (ImGui::BeginPopupContextItem(("##ctx_" + mob).c_str())) {
    				if (ImGui::MenuItem("Remove rule")) {
    					rules.erase(mob);
    					ruleOrder.erase(ruleOrder.begin() + i);
    					if (selectedRuleIdx >= static_cast<int>(ruleOrder.size()))
    						selectedRuleIdx = static_cast<int>(ruleOrder.size()) - 1;
    					ImGui::EndPopup();
    					break;  // break loop after erase to avoid invalid iterator
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
	        const std::string& mob = ruleOrder[selectedRuleIdx];

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
			] = rules[mob];

	        ImGui::Text("Mob: %s", mob.c_str());
	        ImGui::Separator();

	        // --- Theme (required pair: theme + instrument) ---
	        ImGui::Checkbox("Add Theme", &addTheme);
	        if (addTheme) {
	            if (!midiFiles.empty()) {
	                if (themeIdx < 0) themeIdx = 0;

	                std::vector<const char*> items;
	                items.reserve(midiFiles.size());
	                for (auto& f : midiFiles) items.push_back(f.c_str());

	                ImGui::Combo("Theme (MIDI from /input)##ThemeMidi", &themeIdx, items.data(), (int)items.size());
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
			    std::vector<std::string> availableParams;
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
			        std::vector<const char*> items;
			        items.reserve(availableParams.size());
			        for (auto& s : availableParams) {
				        items.push_back(s.c_str());
			        }

			        ImGui::Combo("Select parameter", &addParamIdx, items.data(), static_cast<int>(items.size()));

			        if (ImGui::Button("Add##ConfirmParam")) {
			            const std::string& sel = availableParams[addParamIdx];
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
	            std::vector list(kScales.begin(), kScales.end());
	            std::sort(list.begin(), list.end());
	            int idx = 0;
	            for (int i = 0; i < static_cast<int>(list.size()); ++i) {
		            if (list[i] == *scale) {
		            	idx = i;
		            	break;
		            }
	            }
	            std::vector<const char*> items;
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
	            std::vector list(kLeadStyles.begin(), kLeadStyles.end());
	            std::sort(list.begin(), list.end());
	            int idx = 0;
	        	for (int i = 0; i < static_cast<int>(list.size()); ++i) {
	        		if (list[i] == *lead_style) {
	        			idx = i;
	        			break;
	        		}
	        	}
	            std::vector<const char*> items;
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
	            std::vector list(kBassStyles.begin(), kBassStyles.end());
	            std::sort(list.begin(), list.end());
	            int idx = 0;
	        	for (int i = 0; i < static_cast<int>(list.size()); ++i) {
	        		if (list[i] == *bass_style) {
	        			idx = i;
	        			break;
	        		}
	        	}
	            std::vector<const char*> items;
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
	            std::vector list(kDrumPatterns.begin(), kDrumPatterns.end());
	            std::sort(list.begin(), list.end());
	            int idx = 0;
	        	for (int i = 0; i < static_cast<int>(list.size()); ++i) {
	        		if (list[i] == *drum_pattern) {
	        			idx = i;
	        			break;
	        		}
	        	}
	            std::vector<const char*> items;
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
	            rules.erase(mob);
	            ruleOrder.erase(ruleOrder.begin() + selectedRuleIdx);
	            if (selectedRuleIdx >= static_cast<int>(ruleOrder.size())) {
		            selectedRuleIdx = static_cast<int>(ruleOrder.size()) - 1;
	            }
	        }
	    }
	}
	ImGui::EndChild();
}
