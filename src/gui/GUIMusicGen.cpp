#include "GUI.h"

#include "imgui.h"

using namespace std;


void GUI::musicGen() {
	ImGui::Dummy(ImVec2(0, 4));

	// Toolbar
	if (ImGui::Button("Clear")) {
		guiLog.clear();
		lastLogVersion = 0;
	}

	ImGui::SameLine();
	if (ImGui::Button("Copy All")) {
		vector<string> tmp;
		guiLog.snapshot(tmp);
		string joined; joined.reserve(1024);
		for (const auto & s : tmp) {
			joined += s;
			joined += '\n';
		}
		ImGui::SetClipboardText(joined.c_str());
	}

	ImGui::SameLine();
	ImGui::Checkbox("Auto-scroll", &autoScrollLog);
	
	if (ImGui::Button("Quit Music Engine")) {
		cout << "[GUI] Stop Music requested. Quitting...\n";

		stopMusicIfRunning();  // make sure generator thread is stopped
		if (window) glfwSetWindowShouldClose(window, GLFW_TRUE);

		exit(EXIT_SUCCESS);
	}


	// Fill everything below with the log view
	const ImVec2 avail = ImGui::GetContentRegionAvail();  // whatever is left on screen
	ImGui::BeginChild(
		"##LogView",
		ImVec2(avail.x, avail.y),
		true,
		ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar
	);

	// Render lines
	vector<string> lines;
	guiLog.snapshot(lines);

	ImGuiListClipper clipper;
	clipper.Begin(static_cast<int>(lines.size()));
	while (clipper.Step()) {
		for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
			ImGui::TextUnformatted(lines[i].c_str());
		}
	}
	clipper.End();

	// Auto-scroll on new content
	const size_t v = guiLog.getVersion();
	if (autoScrollLog && v != lastLogVersion) {
		ImGui::SetScrollHereY(1.0f);
		lastLogVersion = v;
	}

	ImGui::EndChild();
}
