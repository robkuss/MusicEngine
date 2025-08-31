#pragma once

#include <MidiFile.h>

#include <chrono>
#include <cmath>


#define DOWNBEAT_PROB_THRESHOLD 0	// Stretch notes that are a downbeat with a higher probability than this
#define DOWNBEAT_ACCURACY       5	// If a note happens within this many ms of a downbeat, it is considered to be on the downbeat

inline double DOWNBEAT_STRETCH_TOLERANCE;	// If a note happens within thiS many ms of a downbeat, it can be stretched or squashed to land exactly on the downbeat


using Clock = std::chrono::high_resolution_clock;

struct TimeSignatureInfo {
	int num		= 4;
	int denom	= 4;
	double bpm	= 120.0;
	int tpq		= 480;

	double msPerTick = 0;
	double msPerBeat = 0;
	double msPerMeas = 0;

	void changeTempo(double newTempo);
};

inline void TimeSignatureInfo::changeTempo(const double newTempo) {
	this->bpm		= newTempo;
	this->msPerBeat = 60e6 / newTempo;
	this->msPerTick = this->msPerBeat / this->tpq;
	this->msPerMeas = this->msPerBeat * this->num;
}


inline TimeSignatureInfo extractTimeSignatureInfo(const smf::MidiFile& midiFile) {
	TimeSignatureInfo tsInfo;
	tsInfo.tpq = midiFile.getTicksPerQuarterNote();

	const smf::MidiEventList& events = midiFile[0];
	for (int i = 0; i < events.size(); ++i) {
		const auto& event = events[i];
		if (event.isTempo()) {
			tsInfo.bpm = 60.0 / event.getTempoSeconds();
			tsInfo.msPerBeat = event.getTempoSeconds() * 1000000.0;
			tsInfo.msPerTick = tsInfo.msPerBeat / tsInfo.tpq;
			tsInfo.msPerMeas = tsInfo.msPerBeat * tsInfo.num;
		}
		if (event.isTimeSignature()) {
			tsInfo.num    = event[3];
			tsInfo.denom  = static_cast<int>(std::pow(2, event[4]));
		}
	}
	return tsInfo;
}



/** Temporal position of a MelodyEvent within a melody */
struct MusicTimePoint {
	int measure;
	double offset; 	// offset in ms if the note isn't exactly on-beat
};


/** Get the measure and position within that measure for a given note event within a melody */
inline MusicTimePoint getMTP(
	const Clock::time_point playStartTime,
	const Clock::time_point eventStartTime,
	const TimeSignatureInfo& tsInfo
) {
	const auto durationSinceStart = std::chrono::duration<double, std::milli>(eventStartTime - playStartTime);
	const double eventTimeMs = durationSinceStart.count();

	// Compute which measure it falls into
	const int measure = static_cast<int>(eventTimeMs / tsInfo.msPerMeas);
	const double offset = eventTimeMs - measure * tsInfo.msPerMeas;

	return MusicTimePoint{measure + 1, offset};
}
