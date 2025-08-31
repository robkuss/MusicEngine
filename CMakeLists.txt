cmake_minimum_required(VERSION 4.0.1)

project(MusicMaker)

find_package(lua CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

set(FLUIDSYNTH_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/fluidsynth/include)
set(FLUIDSYNTH_LIBRARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/fluidsynth/lib)

set(RTMIDI_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/rtmidi-6.0.0)

set(MIDIFILE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/midifile/include)
set(MIDIFILE_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/midifile/lib)

add_definitions(-D__WINDOWS_MM__)


# Add the executable target
add_executable(MusicMaker
		src/MusicMaker.cpp
		src/MelodyMaker.cpp
		src/MIDI.cpp
		src/lua/LuaBindings.cpp
		src/game/GameBridge.cpp

		libs/midifile/src/MidiFile.cpp
		libs/midifile/src/MidiEvent.cpp
		libs/midifile/src/MidiEventList.cpp
		libs/midifile/src/MidiMessage.cpp
		libs/midifile/src/Binasc.cpp

		libs/rtmidi-6.0.0/RtMidi.cpp
		src/MIDIFileLoading.cpp
)

# Add /resources, /input and /lua directories
add_custom_command(TARGET MusicMaker POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		${CMAKE_CURRENT_SOURCE_DIR}/resources
		${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources
)
add_custom_command(TARGET MusicMaker POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		${CMAKE_CURRENT_SOURCE_DIR}/input
		${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/input
)

target_compile_definitions(MusicMaker PRIVATE
		LUA_SETUP_SCRIPT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/lua/setup.lua"
		LUA_LOGIC_SCRIPT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/lua/logic.lua"
)

target_include_directories(MusicMaker PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
		${FLUIDSYNTH_INCLUDE_DIR}
		${RTMIDI_INCLUDE_DIR}
		${MIDIFILE_INCLUDE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/libs/json-develop/include
		${CMAKE_CURRENT_SOURCE_DIR}/libs/sol2/include
)

target_link_directories(MusicMaker PRIVATE
		${FLUIDSYNTH_LIBRARY_DIR}
		${MIDIFILE_LIB_DIR}
)

# === Link to the static or dynamic libs ===
target_link_libraries(MusicMaker PRIVATE
		fluidsynth-3
		winmm
		lua
)

if(WIN32)
	target_link_libraries(MusicMaker PRIVATE ws2_32)
endif()
