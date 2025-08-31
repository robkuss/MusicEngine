#pragma once

#include "Timing.h"

#include <MidiFile.h>

#include <chrono>
#include <sstream>
#include <string>
#include <vector>


// MACROS
#define INPUT_DIR     "input/"
#define RESOURCES_DIR "resources/"

#define PHI_32 0x9e3779b9			// Derived from 2^32 * phi (a.k.a. (sqrt(5) - 1) / 2, a.k.a. the golden ratio, a.k.a. the "most irrational" number) (for hashing)

// ANSI escape codes
// #define ERROR		"\033[31m[ERROR] "
// #define END_ERROR	"\033[0m"


// MIDI information
#define MSG_TYPE(x)		(x&  0xF0)	// MIDI status code high nibble (indicates the message type)
#define NOTE_ON			0x90
#define NOTE_OFF		0x80

#define MIN_PAUSE_LENGTH 100		// Musical pauses have to be at least this long (in ms) to count as pauses


enum class Mode { IDLE, RECORD, PLAY };


// Notes and Melodies
inline std::vector<std::string> noteNames = {"C", "C#/Db", "D", "D#/Eb", "E", "F", "F#/Gb", "G", "G#/Ab", "A", "A#/Bb", "B"};

using Note = unsigned char;

constexpr Note NONE  =  0;
constexpr Note START = -1;			// Start token to get a melody started when the buffer isn't full yet
constexpr Note PAUSE = -2;			// For musical silence


inline int getOctave(const Note note) {
	return std::floor(note / 12 - 1);
}

inline std::string getNoteName(const Note note, const bool withOctave = true) {
	switch (note) {
		case START: return "START";
		case PAUSE: return "PAUSE";
		default: {
			const int octave = getOctave(note);
			std::stringstream sstr;
			sstr << noteNames[note % 12];
			if (withOctave) sstr << octave;
			return sstr.str();
		}
	}
}

/** Detects if a note is a real melody note, or instead an invalid note, or a pause */
inline bool isNoActualNote(const Note note) {
	return note < 13 || note > 127;
}

inline Note relativeMinor(const Note majorRoot) {
	return (majorRoot + 9) % 12;
}

inline Note relativeMajor(const Note minorRoot) {
	return (minorRoot + 3) % 12;
}

inline Note transpose(const Note note, const int semitones) {
	const Note targetNote = note + semitones;

	if (targetNote > 127)
		return targetNote - 12;
	if (targetNote < 13)
		return targetNote + 12;
	return targetNote;
}

inline int interval(const Note from, const Note to) {
	return (to - from + 12) % 12;
}


inline int getNumDigits(const int val) {
	return val > 0
		? static_cast<int>(floor(log10(val))) + 1
		: 0;
}

inline int roundInt(const double num) {
	return static_cast<int>(std::round(num));
}


inline std::chrono::microseconds doubleToMs(const double val) {
	return std::chrono::microseconds(static_cast<int64_t>(val));
}

inline Clock::time_point doubleToTimePoint(const double val) {
	return Clock::time_point(doubleToMs(val));
}


template<typename EventHandler>
static void processMidiEvents(smf::MidiFile& midiFile, const TimeSignatureInfo tsInfo, EventHandler handleEvent) {
	double lastEventTimeMs = 0.0;

	for (int track = 0; track < midiFile.getTrackCount(); ++track) {
		for (int event = 0; event < midiFile[track].size(); ++event) {
			const auto& midiEvent = midiFile[track][event];
			if (!(midiEvent.isNoteOn() && midiEvent.getVelocity() > 0)) continue;

			const Note note = static_cast<Note>(midiEvent.getKeyNumber());

			const double eventTimeMs = midiEvent.tick * tsInfo.msPerTick;
			double durationMs = (midiEvent.getLinkedEvent()->tick - midiEvent.tick) * tsInfo.msPerTick;

			const double deltaTimeMs = eventTimeMs - lastEventTimeMs;
			if (deltaTimeMs > MIN_PAUSE_LENGTH)
				handleEvent(PAUSE, doubleToTimePoint(eventTimeMs - deltaTimeMs), deltaTimeMs);

			handleEvent(note, doubleToTimePoint(eventTimeMs), durationMs);
			lastEventTimeMs = eventTimeMs + durationMs;
		}
	}
}
