#include "GUI.h"

#include "imgui.h"

#include <regex>

using namespace std;
namespace fs = filesystem;


void GUI::titleScreen() {
    constexpr ImVec2 buttonSize(200, 80);
    const ImVec2 winSize = ImGui::GetContentRegionAvail();
    const float x = (winSize.x - buttonSize.x) * 0.5f;

	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);

    // New Project
    ImGui::SetCursorPosX(x);
    ImGui::SetCursorPosY((winSize.y - buttonSize.y*2.f - 12.f) * 0.5f);
    if (ImGui::Button("New Project", buttonSize)) {
        rules.clear();
        ruleOrder.clear();
        selectedRuleIdx = -1;
        selectedMidi.clear();
        autoMarkov = true;
        markovOrder = 1;
        guiState = GUIState::RuleDef;
    }

    ImGui::Dummy(ImVec2(0, 12));

    // Load Project
    ImGui::SetCursorPosX(x);
    if (ImGui::Button("Load Project", buttonSize)) {
        string msg;
		const string path = findRulesLua();
        if (path.empty()) {
            msg = "No rules.lua found.\n"
                  "Expected places: " RUNTIME_OUTPUT_DIR "/lua/rules.lua, ./lua/rules.lua, ./rules.lua";

            ImGui::OpenPopup("LoadProjectResult");
        } else {
            if (loadRulesFromLua(path, msg)) {
                guiState = GUIState::RuleDef;
            }
            ImGui::OpenPopup("LoadProjectResult");
        }

        static string lastLoadMsg;
        lastLoadMsg = msg;

        if (ImGui::BeginPopupModal(
            "LoadProjectResult",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
        {
            ImGui::TextWrapped("%s", lastLoadMsg.c_str());
            ImGui::Dummy(ImVec2(0, 8));
            if (ImGui::Button("OK", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }

    if (ImGui::BeginPopupModal(
        "LoadProjectResult",
        nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
    {
        static string lastLoadMsg; // bleibt über Frames erhalten
        ImGui::TextWrapped("%s", lastLoadMsg.c_str());
        ImGui::Dummy(ImVec2(0, 8));
        if (ImGui::Button("OK", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

	ImGui::PopStyleVar(1);
}

bool GUI::loadRulesFromLua(const string& path, string& outMessage) {
    try {
        if (!fs::exists(path)) {
            outMessage = "No rules.lua found under: " + path;
            return false;
        }

        ifstream f(path, ios::binary);
        if (!f) {
            outMessage = "Could not open file: " + path;
            return false;
        }
        string content((istreambuf_iterator(f)), istreambuf_iterator<char>());

        rules.clear();
        ruleOrder.clear();
        selectedRuleIdx = -1;

        {
            regex re(R"re(local\s+main_midi\s*=\s*"((\\.|[^"\\])*)")re");
            smatch m;
            if (regex_search(content, m, re)) {
                // Unescape \" and \\ and \n \t
                string s = m[1].str();
                string out; out.reserve(s.size());
                for (size_t i = 0; i < s.size(); ++i) {
                    if (s[i] == '\\' && i + 1 < s.size()) {
                        char c = s[++i];
                        if (c == 'n') out.push_back('\n');
                        else if (c == 't') out.push_back('\t');
                        else out.push_back(c);
                    } else out.push_back(s[i]);
                }
                selectedMidi = out;
            } else {
                selectedMidi.clear();
            }
        }

        // --- auto markov ---
        {
            regex re(R"(music\.use_auto_markov\(\s*(true|false)\s*\))");
            smatch m;
            if (regex_search(content, m, re)) {
                autoMarkov = m[1].str() == "true";
            }
        }

        // --- markov order ---
        {
            regex re(R"(music\.set_markov_order\(\s*([0-9]+)\s*\))");
            smatch m;
            if (regex_search(content, m, re)) {
                markovOrder = max(1, stoi(m[1].str()));
            }
        }

        // --- add_rule() blocks ---
    	regex reRule(
			R"re(add_rule\(\s*"((\\.|[^"\\])*)"\s*,\s*\{([\s\S]*?)\}\s*\))re",
			regex::ECMAScript | regex::icase | regex::optimize
		);

        auto begin = sregex_iterator(content.begin(), content.end(), reRule);
        auto end   = sregex_iterator();

    	for (auto it = begin; it != end; ++it) {
    		const smatch& m = *it;
    		string trigger = m[1].str();
    		{
    			string s = trigger; trigger.clear(); trigger.reserve(s.size());
    			for (size_t i = 0; i < s.size(); ++i) {
    				if (s[i] == '\\' && i + 1 < s.size())
    					trigger.push_back(s[++i]);
    				else trigger.push_back(s[i]);
    			}
    		}
    		string body = m[3].str();

    		Rule r;

        	// Helpers
        	auto unescape_lua = [](const string& s){
        		string out; out.reserve(s.size());
        		for (size_t i = 0;i < s.size(); ++i){
        			if (s[i] == '\\' && i + 1 < s.size()) {
						const char c = s[++i];
        				if (c == 'n') out.push_back('\n');
        				else if (c == 't') out.push_back('\t');
        				else out.push_back(c);
        			} else out.push_back(s[i]);
        		}
        		return out;
        	};

        	auto findString = [&](const string& key) -> optional<string> {
				const string pat = key + " = ";
        		size_t pos = body.find(pat);
        		if (pos == string::npos) return nullopt;
        		pos = body.find('"', pos);
        		if (pos == string::npos)
					return nullopt;
				const size_t e = body.find('"', pos+1);
        		if (e == string::npos)
					return nullopt;
				const string raw = body.substr(pos+1, e-pos-1);
        		return unescape_lua(raw);
        	};

        	auto findInt = [&](const string& key) -> optional<int> {
        		const regex re(key + R"(\s*=\s*(-?\d+))");
        		smatch match;
        		if (regex_search(body, match, re))
        			return stoi(match[1].str());
        		return nullopt;
        	};

        	auto findFloat = [&](const string& key) -> optional<float> {
        		const regex re(key + R"(\s*=\s*(-?[0-9]*\.?[0-9]+))");
        		smatch match;
        		if (regex_search(body, match, re))
        			return stof(match[1].str());
        		return nullopt;
        	};


        	if (auto th = findString("theme")) {
        		r.addTheme = true;
        		r.theme = *th;
        	}
        	if (auto ins = findInt ("instrument")) {
        		r.instrument = clamp(*ins,0,127);
        		r.addTheme = true;
        	}

        	r.intensity              = findFloat ("intensity");
        	if (auto sc = findString("scale"))            r.scale          = *sc;
        	r.tempo_multiplier       = findFloat ("tempo_multiplier");
        	if (auto ls = findString("lead_style"))       r.lead_style     = *ls;
        	if (auto ll   = findInt   ("lead_layers"))      r.lead_layers = max(1,*ll);
        	if (auto cl   = findInt   ("chord_layers"))     r.chord_layers= max(1,*cl);
        	if (auto bs = findString("bass_style"))       r.bass_style     = *bs;
        	if (auto dp = findString("drum_pattern"))     r.drum_pattern   = *dp;

    		if (!r.theme.empty()) {
    			int idx = -1;
    			for (int i = 0; i < static_cast<int>(midiFiles.size()); ++i) {
    				if (midiFiles[i] == r.theme ||
						filesystem::path(midiFiles[i]).filename() == filesystem::path(r.theme).filename()) {
    					idx = i; break;
					}
    			}
    			r.themeIdx = idx;
    		}

    		string trigCanon = canonicalize(trigger);

    		auto type = TriggerType::Mob;  // default
    		if (ranges::find(selectedGame.envList, trigCanon) != selectedGame.envList.end()) {
    			type = TriggerType::Environment;
    		} else if (ranges::find(selectedGame.tagsList, trigCanon) != selectedGame.tagsList.end()) {
    			type = TriggerType::Tag;
    		}

    		// Build key with canonical name
    		TriggerKey key{type, trigCanon};

    		rules[key] = move(r);
    		ruleOrder.push_back(key);
        }

        if (!ruleOrder.empty()) selectedRuleIdx = 0;

        outMessage = "Loaded project from: " + path;
        return true;
    } catch (const exception& e) {
        outMessage = string("Error while loading: ") + e.what();
        return false;
    }
}

string GUI::findRulesLua() const {
	if (fs::exists(outPath)) return outPath;

	const char* candidates[] = {
		RUNTIME_OUTPUT_DIR "/lua/rules.lua",
		"./lua/rules.lua",
		"./rules.lua"
	};
	for (auto* c : candidates) {
		try {
			if (fs::exists(c)) return c;
		} catch (...) {}
	}

	auto try_scan = [](const fs::path& root) -> string {
		try {
			if (!exists(root)) return {};
			for (auto it = fs::recursive_directory_iterator(root);
				it != fs::recursive_directory_iterator(); ++it) {
				if (it.depth() > 2) {
					it.disable_recursion_pending();
					continue;
				}
				if (!it->is_regular_file()) continue;
				if (it->path().filename() == "rules.lua")
					return it->path().string();
			}
		} catch (...) {}
		return {};
	};

	if (auto p = try_scan("./lua"); !p.empty()) return p;
	if (auto p = try_scan("."); !p.empty()) return p;

	return {};
}
