#pragma once

constexpr bool NOTE_OUTPUT = false;


inline std::string formatMTP(
	const MusicTimePoint& mtp,
	const int mdMeasNum,
	const bool markDownbeat,
	const bool showPrefix
) {
	using namespace std;

	stringstream sstr;
	if (showPrefix) sstr << "Time point: ";

	sstr << format("m. {:<{}}", mtp.measure, mdMeasNum);

	if (mtp.offset >= (markDownbeat ? 1 : DOWNBEAT_ACCURACY)) {
		sstr << " " << format("(+{:>4} ms)", roundInt(mtp.offset));
	} else if (markDownbeat) {
		sstr << " [DOWNBEAT]";
	}

	return sstr.str();
}


inline void printFixedEvent(
	int i,
	const FixedEvent* event,
	const int mdNoteNum,
	const int mdMeasNum
) {
	using namespace std;

	cout << format("{:<{}} | {:<6} | {} | Duration: {:>4} ms",
		format("#{0:0{1}}", i, mdNoteNum),
		mdNoteNum + 1,
		getNoteName(event->note),
		formatMTP(event->mtp, mdMeasNum, true, true),
		static_cast<int>(round(event->duration / 1000.0))
	) << endl;
}

inline void printScheduledEvent(const std::shared_ptr<ScheduledEvent> &event) {
	using namespace std;

	if constexpr (!NOTE_OUTPUT)
		return;

	cout << format(
		"{:<26}", format("Playing {:<6} for {:>4} ms",
		getNoteName(event->note), static_cast<int>(round(event->duration))))
	<< endl;
}


inline void printMelodyInformation(const Melody& melody) {
	using namespace std;

	cout << "\n=== Extracted MIDI Events ===\n";
	if (melody.events.empty()) {
		cout << "No MIDI events!\n";
		return;
	}

	const int mdNoteNum = getNumDigits(static_cast<int>(melody.events.size()));
	const int measNum   = dynamic_cast<FixedEvent*>(melody.events.back().get())->mtp.measure;
	const int mdMeasNum = getNumDigits(measNum);

	int i = 0;
	for (const auto& evPtr : melody.events) {
		const FixedEvent* full = dynamic_cast<FixedEvent*>(evPtr.get());
		if (!full) continue;

		printFixedEvent(++i, full, mdNoteNum, mdMeasNum);
	}

	cout << "\nShortest note length: " << melody.shortestNoteLength / 1e3 << "ms\n";
}


inline void printTimeSignatureInformation(const TimeSignatureInfo& tsInfo) {
	using namespace std;

	cout << "\n--- Time Signature&  Tempo Info ---" << endl;
	cout << "Time Signature: " << tsInfo.num << "/" << tsInfo.denom << endl;
	cout << "BPM: " << tsInfo.bpm << endl;
}


/** Rounds an input to one decimal point and checks if it's significantly different from the previous value */
inline void printIfChanged(const std::string& label, double newValue, double oldValue) {
	using namespace std;

	const string newString = format("{:.1f}", newValue);
	const string oldString = format("{:.1f}", oldValue);
	if (newString != oldString) {
		cout << label << ": " << newString << endl;
	}
}
