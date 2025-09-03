cmake_minimum_required(VERSION 4.0.1)

project(MusicEngine)


find_package(lua CONFIG REQUIRED)
find_package(OpenGL REQUIRED)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

set(FLUIDSYNTH_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/fluidsynth/include)
set(FLUIDSYNTH_LIBRARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/fluidsynth/lib)

set(RTMIDI_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/rtmidi-6.0.0)

set(MIDIFILE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/midifile/include)
set(MIDIFILE_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/midifile/lib)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)


add_subdirectory(libs/glfw-3.4 ${CMAKE_BINARY_DIR}/glfw-build)

add_definitions(-D__WINDOWS_MM__)

add_definitions(-DRUNTIME_OUTPUT_DIR="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

add_library(glad STATIC libs/glad/src/glad.c)
target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/glad/include)


add_library(imgui
		libs/imgui/imgui.cpp
		libs/imgui/imgui_draw.cpp
		libs/imgui/imgui_tables.cpp
		libs/imgui/imgui_widgets.cpp
		libs/imgui/backends/imgui_impl_glfw.cpp
		libs/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui
		${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui/backends
)

target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)
target_link_libraries(imgui PUBLIC glfw glad OpenGL::GL)


# Add the executable target
add_executable(MusicEngine
		src/MusicEngine.cpp
		src/MusicMaker.cpp
		src/MelodyMaker.cpp
		src/MIDI.cpp
		src/MIDIFileLoading.cpp
		src/lua/LuaBindings.cpp
		src/game/GameBridge.cpp
		src/game/GameStateHandler.cpp
		src/gui/GUI.cpp
		src/gui/GUITitleScreen.cpp
		src/gui/GUIRuleDef.cpp

		test/PerformanceTesting.cpp

		libs/midifile/src/MidiFile.cpp
		libs/midifile/src/MidiEvent.cpp
		libs/midifile/src/MidiEventList.cpp
		libs/midifile/src/MidiMessage.cpp
		libs/midifile/src/Binasc.cpp
		src/gui/GUIMusicGen.cpp
)


# Add /resources, /input and /lua directories
add_custom_command(TARGET MusicEngine POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		${CMAKE_CURRENT_SOURCE_DIR}/resources
		${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources
)
add_custom_command(TARGET MusicEngine POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		${CMAKE_CURRENT_SOURCE_DIR}/input
		${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/input
)
add_custom_command(TARGET MusicEngine POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		${CMAKE_CURRENT_SOURCE_DIR}/lua
		${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lua
)

target_include_directories(MusicEngine PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
		${CMAKE_CURRENT_SOURCE_DIR}/libs/json-develop/include
		${CMAKE_CURRENT_SOURCE_DIR}/libs/sol2/include
		${FLUIDSYNTH_INCLUDE_DIR}
		${RTMIDI_INCLUDE_DIR}
		${MIDIFILE_INCLUDE_DIR}
)

target_link_directories(MusicEngine PRIVATE
		${FLUIDSYNTH_LIBRARY_DIR}
		${MIDIFILE_LIB_DIR}
)

# === Link to the static or dynamic libs ===
target_link_libraries(MusicEngine PRIVATE
		fluidsynth-3
		winmm
		lua
		glad
		imgui
		OpenGL::GL
)

if(WIN32)
	target_link_libraries(MusicEngine PRIVATE ws2_32)
endif()
