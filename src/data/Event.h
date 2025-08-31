#pragma once

#include "util/Util.h"
#include "util/Timing.h"


/** An event (note, pause, etc.) within a melody */
struct Event {
	Note note{};

	virtual ~Event() = default;  // Enables safe dynamic_cast

	bool operator==(const Event& other) const {
		return note == other.note;
	}
};

/** For training purposes, just note information without timing */
struct SimpleEvent final : Event {
	explicit SimpleEvent(const Note note) {
		this->note = note;
	}
};

/** Event information with fixed music time point (mtp) */
struct FixedEvent final : Event {
	MusicTimePoint mtp{};	// Start time within the melody
	double duration;		// In microseconds

	FixedEvent(
		const Note note,
		const MusicTimePoint& mtp,
		const double duration
	) : mtp(mtp), duration(duration) {
		this->note = note;
	}
};

/** Event information with scheduled global time point */
struct ScheduledEvent final : Event {
	Clock::time_point startTime;  // Start time within the melody
	double duration;			  // In microseconds

	ScheduledEvent(
		const Note note,
		const Clock::time_point& startTime,
		const double duration
	) : startTime(startTime), duration(duration) {
		this->note = note;
	}
};

struct ScheduledPlaybackEvent {
	Clock::time_point startTime;
	Note note{};
	int channel{};
	int velocity{};
	bool isNoteOn{};
};


using MelodyEventPtr = std::shared_ptr<Event>;

inline bool operator==(const MelodyEventPtr& a, const MelodyEventPtr& b) {
	return a->note == b->note;
}


// Custom hashes
namespace std {
	template<>
	struct hash<Event> {
		size_t operator()(const Event& ev) const noexcept {
			return hash<Note>()(ev.note);
		}
	};

	template<>
	struct hash<SimpleEvent> {
		size_t operator()(const SimpleEvent& ev) const noexcept {
			return hash<Note>()(ev.note);
		}
	};

	template<>
	struct hash<FixedEvent> {
		size_t operator()(const FixedEvent& ev) const noexcept {
			return hash<Note>()(ev.note) + PHI_32;
		}
	};


	template <>
	struct hash<MelodyEventPtr> {
		size_t operator()(const MelodyEventPtr& ev) const noexcept {
			return std::hash<Note>()(ev->note);
		}
	};
}

template <typename T>
	struct VectorHash {
	size_t operator()(const std::vector<T>& vec) const {
		size_t seed = vec.size();
		for (const auto& item : vec) {
			seed ^= std::hash<T>()(item) + PHI_32 + (seed << 6) + (seed >> 2);
		}
		return seed;
	}
};


struct Melody {
	std::vector<MelodyEventPtr> events;  // Shared pointers to prevent object slicing

	mutable Note keyRoot;
	mutable double shortestNoteLength;
};


/** Helper to extract note values from a Melody */
inline std::vector<Note> flattenNotes(const Melody& melody) {
	std::vector<Note> result;
	result.reserve(melody.events.size());
	for (const auto& event : melody.events) {
		result.push_back(event->note);  // Access note from base class
	}
	return result;
}
