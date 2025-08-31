#pragma once

#include <bits/ranges_algo.h>
#include <cfloat>

#include "data/Event.h"
#include "data/Scale.h"


struct KeyCandidate {
	Note key;
	int rootNoteCount;
};

struct MelodyNoteStat {
	Note note{};
	int count{};
	double percentage{};
};

struct KeyDetectionResult {
	Note bestKey = NONE;
	std::vector<MelodyNoteStat> stats;
	std::vector<KeyCandidate> candidates;
	Note mostFrequentNote = NONE;
};


inline KeyDetectionResult detectKey(const std::vector<Note>& notes) {
	std::array<int, 12> counts{};
	for (const Note& note : notes) counts[note]++;

	std::vector<MelodyNoteStat> stats;
	for (Note i = 0; i < 12; ++i) {
		if (counts[i] == 0) continue;
		double percentage = static_cast<double>(counts[i]) / static_cast<double>(notes.size()) * 100.0;
		stats.emplace_back(i, counts[i], percentage);
	}

	std::ranges::sort(stats, [](const auto& a, const auto& b) {
		return a.count > b.count;
	});

	const Note mfNote = stats.empty() ? NONE : stats[0].note;

	std::vector<KeyCandidate> candidates;
	for (unsigned char key = 0; key < 12; ++key) {
		auto scale = buildScale(key, Scale::IONIAN);

		std::array<bool, 12> isInScale{};
		for (const Note n: scale)
			isInScale[n] = true;

		const bool allFit = std::ranges::all_of(stats, [&](const MelodyNoteStat& stat) {
			return isInScale[stat.note];
		});

		if (allFit) candidates.push_back({key, counts[key]});
	}

	Note bestKey = NONE;
	if (!candidates.empty()) {
		if (candidates.size() == 1) {
			bestKey = candidates[0].key;
		} else {
			const auto it = std::ranges::find_if(candidates, [&](const KeyCandidate& c) {
				return c.key == mfNote;
			});
			bestKey = it != candidates.end()
				? it->key
				: std::ranges::max(candidates, {},& KeyCandidate::rootNoteCount).key;
		}
		[[maybe_unused]] volatile auto _ = bestKey;
	}

	return KeyDetectionResult{
		.bestKey = bestKey,
		.stats = std::move(stats),
		.candidates = std::move(candidates),
		.mostFrequentNote = mfNote
	};
}

inline KeyDetectionResult detectKey(smf::MidiFile &midiFile, const TimeSignatureInfo &themeTs) {
	std::vector<Note> notes;
	processMidiEvents(midiFile, themeTs, [&]<typename T>(const T& noteOrPause, const Clock::time_point, double) {
		if (!isNoActualNote(noteOrPause)) {
			notes.push_back(noteOrPause % 12);  // Discard octave
		}
	});
	return detectKey(notes);
}

inline KeyDetectionResult detectKey(const Melody& melody) {
	std::vector<Note> notes;
	for (const auto& event : melody.events) {
		const Note note = event->note;
		if (isNoActualNote(note)) continue;
		notes.push_back(note % 12);  // Discard octave
	}
	return detectKey(notes);
}


inline void printKeyDetectionDebug(const KeyDetectionResult& result) {
	using namespace std;

	if (result.stats.empty()) {
		cout << "No notes to analyze." << endl;
		return;
	}

	const auto& stats = result.stats;
	const auto& candidates = result.candidates;

	int mdCount = getNumDigits(stats[0].count);
	int mdPercentage = getNumDigits(static_cast<int>(stats[0].percentage)) + 3;

	cout << "\n--- Note Occurrence Statistics ---" << endl;
	for (const auto& [noteIndex, count, percentage] : stats) {
		cout << std::format("{:<6}: {:>{}} ({:>{}.2f}%)",
			getNoteName(noteIndex, false),
			count,
			mdCount,
			percentage,
			mdPercentage
		) << endl;
	}

	if (candidates.empty()) {
		cout << "\nNo matching key found (some notes don't fit any major scale)." << endl;
		return;
	}

	if (candidates.size() == 1) {
		const Note key = candidates[0].key;
		cout << "\nKey:\n"
			 << getNoteName(key, false) << " major / " << getNoteName(relativeMinor(key), false) << " minor"
			 << endl;
		return;
	}

	cout << "\nMost fitting key:\n"
		 << getNoteName(result.bestKey, false) << " major / "
		 << getNoteName(relativeMinor(result.bestKey), false) << " minor"
		 << endl;

	cout << "\nOther candidates considered:" << endl;

	const int bestCount = std::ranges::find(candidates, result.bestKey,& KeyCandidate::key)->rootNoteCount;

	auto likelihood = [](const double ratio) {
		return ratio < 0.5
			? (ratio < 0.2 ? "very unlikely" : "unlikely")
			: ratio > 0.8 ? "very likely" : "somewhat likely";
	};

	for (const auto& [key, rootNoteCount] : candidates) {
		if (key == result.bestKey)
			continue;

		const double ratio = static_cast<double>(rootNoteCount) / bestCount;
		cout << std::format("{} major / {} minor ({})",
				getNoteName(key, false),
				getNoteName(relativeMinor(key), false),
				likelihood(ratio))
			<< endl;
	}
}


inline void updateMelodyMetadata(const Melody& melody, KeyDetectionResult* outResult = nullptr) {
	auto shortestNoteLength = DBL_MAX;

	for (const auto& evPtr: melody.events) {
		const FixedEvent *full = dynamic_cast<FixedEvent *>(evPtr.get());
		if (!full) continue;

		if (full->duration < shortestNoteLength)
			shortestNoteLength = full->duration;
	}
	melody.shortestNoteLength = shortestNoteLength;

	const KeyDetectionResult result = detectKey(melody);
	melody.keyRoot = result.bestKey;

	if (outResult)
		*outResult = result;
}


inline void fitToChord(smf::MidiFile& midiFile, const Note themeKey, const Chord& chord) {
	const auto chordQuality= basicQuality(chord);
	const auto chordRoot = chordQuality == "minor"
		? chord.root + 3
		: chord.root;

	const int semitoneShift = interval(themeKey, chordRoot);
	std::cout << "Semitone shift: " << semitoneShift << std::endl;

	for (int track = 0; track < midiFile.getTrackCount(); ++track) {
		for (int event = 0; event < midiFile[track].size(); ++event) {
			auto& midiEvent = midiFile[track][event];

			if (midiEvent.isNoteOn() || midiEvent.isNoteOff()) {
				const int note = midiEvent.getKeyNumber();
				int newNote = transpose(note, semitoneShift);
				const auto intervalFromChordRoot = interval(chordRoot, newNote % 12);

				if (midiEvent.isNoteOn()) std::cout << "Chord root: " << chordRoot << ", note: " << newNote % 12 << ", interval: " << intervalFromChordRoot << std::endl;

				if (chordQuality == "sus") {
					const auto susInterval = (intervalFromChordRoot - 3) % 12;
					bool keepNote = susInterval == 0 || susInterval == 7;
					if (chord.type.quality == "sus2" && susInterval == 2) keepNote = true;
					if (chord.type.quality == "sus4" && susInterval == 5) keepNote = true;

					if (!keepNote) newNote = transpose(newNote, -intervalFromChordRoot);
				}

				midiEvent.setKeyNumber(newNote);
			}
		}
	}
}
