#include "GUI.h"

#include <iostream>
#include <ranges>

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"

using namespace std;


string toLower(string s) {
	transform(s.begin(), s.end(), s.begin(), [](const unsigned char c) {
		return static_cast<char>(tolower(c));
	});
	return s;
}

// Quoted Lua-String
string luaQuote(const string& s) {
	string out; out.reserve(s.size() + 2);
	out.push_back('"');
	for (const char c : s) {
		if (c == '\\' || c == '"') {
			out.push_back('\\');
			out.push_back(c);
		}
		else if (c == '\n') out += "\\n";
		else if (c == '\t') out += "\\t";
		else out.push_back(c);
	}
	out.push_back('"');
	return out;
}


void errorCallback(const int code, const char* msg) {
	cerr << "[GLFW] error " << code << ": " << msg << "\n";
}

int windowWidth, windowHeight;
void windowSizeCallback(GLFWwindow* window, const int width, const int height) {
	windowWidth = width;
	windowHeight = height;
}


GUI::GUI(
	const string &title,
	const int width,
	const int height
) : title(title), width(width), height(height), selectedGame() {
	windowWidth = width;
	windowHeight = height;

	snprintf(outPath, sizeof(outPath), "%s/lua/rules.lua", RUNTIME_OUTPUT_DIR);

	selectedGame = Minecraft;

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

	window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
	if (!window) {
		cerr << "Window creation failed\n";
		glfwTerminate();
		return;
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);  // VSync

	glfwSetWindowSizeCallback(window, windowSizeCallback);

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
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");  // Shader version


	// Redirect cout / cerr to GUI log
	coutRedirect = make_unique<GuiLogStreamBuf>(guiLog, "out");
	oldCoutBuf = cout.rdbuf(coutRedirect.get());

	cerrRedirect = make_unique<GuiLogStreamBuf>(guiLog, "err");
	oldCerrBuf = cerr.rdbuf(cerrRedirect.get());


	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		// Begin ImGui Frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);

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
			case GUIState::RuleDef:
				ruleDef();
				break;
			case GUIState::MusicGen:
				musicGen();
				break;
			default: break;
		}

		ImGui::End();

		// Prepare render
		ImGui::Render();
		int fb_w, fb_h;
		glfwGetFramebufferSize(window, &fb_w, &fb_h);
		glViewport(0, 0, fb_w, fb_h);
		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Render ImGui DrawData
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	// Cleanup
	stopMusicIfRunning();

	// Restore original stream buffers
	if (oldCoutBuf) cout.rdbuf(oldCoutBuf), oldCoutBuf = nullptr;
	if (oldCerrBuf) cerr.rdbuf(oldCerrBuf), oldCerrBuf = nullptr;

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	window = nullptr;
	glfwTerminate();
}


void GUI::stopMusicIfRunning() {
	if (musicRunning.load(memory_order_acquire)) {
		if (music) music->requestStop();
		if (musicThread.joinable()) musicThread.join();
		music.reset();
		musicRunning.store(false, memory_order_release);
		musicPaused.store(false, memory_order_release);
	}
}

void GUI::exportLua(const string& path) {
    try {
        filesystem::path p(path);
        if (p.has_parent_path()) create_directories(p.parent_path());

        // Fallback
        string mainMidi = selectedMidi;
        if (mainMidi.empty() && !midiFiles.empty()) mainMidi = midiFiles.front();

        ostringstream ss;
        ss << "-- rules.lua\n\n";
        if (!mainMidi.empty()) {
            ss << "local main_midi = " << luaQuote(mainMidi) << "\n";
            ss << "music.preload_midi(main_midi)\n";
            ss << "music.set_main_midi(main_midi)\n";
        } else {
            ss << "local main_midi = \"\"\n";
        }
        ss << "music.use_auto_markov(" << (autoMarkov ? "true" : "false") << ")\n";
        ss << "music.set_markov_order(" << markovOrder << ")\n\n";

        ss << "rules = {}\n\n";
        ss << "function add_rule(trigger, config)\n"
              "    rules[trigger] = config\n"
              "end\n\n";

        ss << "-- Rule definitions\n\n";

    	vector<TriggerKey> keys;
    	keys.reserve(rules.size());
    	for (const auto &k: rules | views::keys) {
    		keys.push_back(k);
    	}

    	// Sort: first by kind (Mob < Environment < Tag), then by name
    	sort(keys.begin(), keys.end(),
			[](const TriggerKey& a, const TriggerKey& b) {
				if (a.type != b.type)
					return static_cast<int>(a.type) < static_cast<int>(b.type);
				return a.name < b.name;
			});

    	for (const auto& key : keys) {
            const Rule& r = rules.at(key);
            vector<string> fields;

            if (r.addTheme && !r.theme.empty()) {
                fields.push_back("theme = " + luaQuote(r.theme));
                fields.push_back("instrument = " + to_string(r.instrument));
            }

            if (r.intensity)    fields.push_back("intensity = "	 + to_string(*r.intensity));
            if (r.scale)        fields.push_back("scale = "		 + luaQuote(*r.scale));
            if (r.tempo_multiplier) {
                ostringstream f; f << setprecision(6) << *r.tempo_multiplier;
                fields.push_back("tempo_multiplier = " + f.str());
            }
            if (r.lead_style)   fields.push_back("lead_style = "   + luaQuote(*r.lead_style));
            if (r.lead_layers)  fields.push_back("lead_layers = "  + to_string(*r.lead_layers));
            if (r.chord_layers) fields.push_back("chord_layers = " + to_string(*r.chord_layers));
            if (r.bass_style)   fields.push_back("bass_style = "   + luaQuote(*r.bass_style));
            if (r.drum_pattern) fields.push_back("drum_pattern = " + luaQuote(*r.drum_pattern));

    		ss << "add_rule(" << luaQuote(key.name) << ", {\n";
            if (!fields.empty()) {
                for (size_t i = 0; i < fields.size(); ++i) {
                    ss << "    " << fields[i];
                    if (i + 1 < fields.size()) ss << ",";
                    ss << "\n";
                }
            }
            ss << "})\n\n";
        }

        ofstream f(path, ios::binary);
        if (!f) return;
        f << ss.str();
    } catch (const exception& e) {
        cerr << "Export error: " << e.what() << "\n";
    }
}
