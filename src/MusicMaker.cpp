#include "MusicMaker.h"

#include <cfloat>

#include "util/Debug.h"

using namespace std;
using namespace smf;

constexpr bool INIT_DEBUG = false;
constexpr bool DEBUG_JSON = false;

constexpr bool OFFLINE_MODE = false;  // Doesn't require connection to a game to generate music


void MusicMaker::start() {
	// Load MIDI soundfont
	midi.loadSoundfont("soundfonts/FluidR3_GM.sf2");

	// Initialize instrument channels
	midi.selectProgram(LEAD.channel,   LEAD.program);
	midi.selectProgram(CHORDS.channel, CHORDS.program);
	midi.selectProgram(BASS.channel,   BASS.program);

	// Load Lua bindings
	bindMusicFunctions();

	// Load setup.lua for input MIDI file and user-defined rules
	loadLuaRules();
	validateAllRules();

	// Get input training MIDI
	MidiFile mainMIDIFile = getCachedMIDI(mainMIDIFilePath);

	// Extract bpm and time signature
	tsInfo = extractTimeSignatureInfo(mainMIDIFile);
	originalBpm = tsInfo.bpm;

	// Generate the drum pattern map
	drumPatterns = getDrumPatterns(tsInfo);

	// Extract melody
	processMidiEvents(mainMIDIFile, tsInfo, [&](
		const Note note,
		const Clock::time_point eventStartTime,
		const double duration
	) {
		const auto event = make_shared<FixedEvent>(
			note,
			getMTP(playStartTime, eventStartTime, tsInfo),
			duration
		);

		melody.events.push_back(event);
	});

	KeyDetectionResult result;
	updateMelodyMetadata(melody, INIT_DEBUG ? &result : nullptr);

	if (INIT_DEBUG) {
		printTimeSignatureInformation(tsInfo);
		printMelodyInformation(melody);
		printKeyDetectionDebug(result);
		cout << "=============================\n";
	}

	// Dynamically set tolerance for downbeat detection
	DOWNBEAT_STRETCH_TOLERANCE = abs(melody.shortestNoteLength - 5);

	// Set Markov chain order (a.k.a. lookbehind)
	const int markovChainOrder = autoMarkov
		? determineBestOrder(melody, mode)
		: markovOrder;

	// Initialize Markov chain and note buffer for training
	mm.initMarkovChain(markovChainOrder, melody);

	// Load logic.lua and bind remaining music functions
	loadLuaLogic();

	// Initialize music state
	onStart();

	if constexpr (!OFFLINE_MODE) {
		// Start the socket server
		gb.startGameStateListener();
		gb.waitForConnection();

		startGameStateThread();
	}

	// Start generating music
	play();
}


void MusicMaker::play() {
	mode = Mode::PLAY;

	double playTime = 0.0;  // Accumulated playing time
	double mstRel   = 0.0;  // Relative measure start time (relative to playStartTime)

	Chord lastChord{};

	vector<ScheduledPlaybackEvent> overflowQueue;  // For events that drag over to the next measure

	// Short delay to ensure that the default settings have been replaced
	this_thread::sleep_for(chrono::milliseconds(150));

	playStartTime = Clock::now();

	// Generate music measure by measure
	while (true) {
		if constexpr (!OFFLINE_MODE) {
			if (!gb.isClientConnected()) {
				cout << "[MusicMaker] Stopping music...\n";
				midi.stopAll();
				break;
			}

			if (isPaused) {
				this_thread::sleep_for(chrono::milliseconds(10));
				continue;
			}
		}

		// --- SETUP ---
		playbackQueue = move(overflowQueue);
		overflowQueue.clear();

		// Change tempo
		currentBpm = originalBpm * musicState.tempoMultiplier;
		tsInfo.changeTempo(currentBpm);

		// Timing (after tempo change!)
		const double metRel = mstRel + tsInfo.msPerMeas;  // relative measure end time
		const auto mstAbs = playStartTime + doubleToMs(mstRel);  // absolute measure start time
		const auto metAbs = playStartTime + doubleToMs(metRel);  // absolute measure end time


		// Generate chords for current scale (i.e. musical mode)
		generateDiatonicChords(melody.keyRoot, musicState.scale);


		// --- GENERATE EVENTS ---
		// Generate melody events
		vector<shared_ptr<ScheduledEvent>> schedule;

		while (playTime < metRel) {
			auto nextEvent = pollNextEventWithTiming(playTime);
			if (!nextEvent) continue;

			// Clip event duration to fit in measure if the overhang is reasonably small (less than an 16th note)
			const double remaining = metRel - (playTime - mstRel);
			if (nextEvent->duration >= remaining && nextEvent->duration < remaining + 0.25 * tsInfo.msPerBeat) {
				nextEvent->duration = remaining;
			}

			schedule.push_back(nextEvent);
			playTime += nextEvent->duration;
		}


		// Chord detection
		vector<Note> notesInMeasure;
		for (const auto& e: schedule)
			if (!isNoActualNote(e->note))
				notesInMeasure.push_back(e->note);

		Chord nextChord = notesInMeasure.empty() ? lastChord : getChord(notesInMeasure, musicState.scale);

		playChordTransition(lastChord, nextChord);
		lastChord = nextChord;


		const auto themeStartTime = mstAbs;

		// Add active MIDI themes to the music
		for (const auto& [path, instrument] : activeThemes) {
			auto midiFile = getCachedMIDI(path);

			// Fit theme to tempo
			TimeSignatureInfo themeTs = extractTimeSignatureInfo(midiFile);
			themeTs.changeTempo(currentBpm);

			// Fit theme to chord
			const KeyDetectionResult kdr = detectKey(midiFile, themeTs);
			const Note themeKey = kdr.bestKey;
			fitToChord(midiFile, themeKey, nextChord);

			auto &[msOffset] = themePlaybackStates[path];  // Per-theme playhead tracker
			const double themeLengthMs = midiFile.getFileDurationInTicks() * themeTs.msPerTick;

			processMidiEvents(midiFile, themeTs, [&]<typename T>(const T& noteOrPause, const Clock::time_point eventTimeRaw, double durationMs) {
				// Get time since the start of this theme parse
				const auto eventTime = themeStartTime + (eventTimeRaw - Clock::time_point{});  // realign to current measure
				const double localMs = chrono::duration<double, micro>(eventTime - themeStartTime).count();

				double loopedMs = fmod(localMs, themeLengthMs);
				if (loopedMs < 0.0)
					loopedMs += themeLengthMs;

				if (loopedMs < msOffset || loopedMs >= msOffset + tsInfo.msPerMeas)
					return;

				const double offsetWithinWindow = loopedMs - msOffset;
				const auto scheduledTime = themeStartTime + doubleToMs(offsetWithinWindow);

				if constexpr (is_same_v<T, Note>) {
					scheduleNote(noteOrPause, scheduledTime, durationMs, instrument);
				}
			});

			// Advance theme playhead
			msOffset += tsInfo.msPerMeas;

			// Wrap around if needed
			if (msOffset >= themeLengthMs)
				msOffset = fmod(msOffset, themeLengthMs);
		}


		// --- SCHEDULE MIDI EVENTS ---
		scheduleMelody(schedule);

		scheduleBass(nextChord, mstAbs);

		scheduleDrums(mstAbs);


		// --- PLAY EVENTS ---
		// Filter playbackQueue to only current measure
		vector<ScheduledPlaybackEvent> currentMeasureEvents;
		for (const auto& evt: playbackQueue) {
			if (evt.startTime <= metAbs)
				currentMeasureEvents.push_back(evt);
			else
				overflowQueue.push_back(evt);
		}


		// Play the events in proper order
		ranges::sort(currentMeasureEvents, {}, &ScheduledPlaybackEvent::startTime);
		for (const auto& [time, note, channel, velocity, isNoteOn]: currentMeasureEvents) {
			this_thread::sleep_until(time);

			// ReSharper disable once CppDFAConstantConditions
			// ReSharper disable once CppDFAUnreachableCode
			if (isPaused)
				break;

			if (isNoteOn)
				midi.playNote(note, channel, velocity);
			else
				midi.stopNote(note, channel);
		}

		mstRel += tsInfo.msPerMeas;
	}
}


/** Get the next melody event from the MelodyMaker's Markov model and adjust its timing */
shared_ptr<ScheduledEvent> MusicMaker::pollNextEventWithTiming(const double melodyTime) {
	const auto event = mm.pollNextEvent();

	// Skip START tokens
	const auto full = dynamic_pointer_cast<FixedEvent>(event);
	if (!full) return {};

	// Adjust note based on current musical mode (church scale)
	Note note = changeNoteForScale(full->note, melody.keyRoot, musicState.scale, musicState.scale);

	// Compute initial target time
	auto targetTime = playStartTime + doubleToMs(melodyTime);

	// Scale duration based on dynamic tempo
	double duration = full->duration * (originalBpm / tsInfo.bpm);

	return make_shared<ScheduledEvent>(
		note,
		targetTime,
		duration
	);
}


/** Schedule a single note on-off pair */
void MusicMaker::scheduleNote(
	const Note note,
	const Clock::time_point startTime,
	const double duration,
	const ActiveInstrument instrument
) {
	playbackQueue.emplace_back(startTime, note, instrument.channel, instrument.velocity, true);
	playbackQueue.emplace_back(startTime + doubleToMs(duration), note, instrument.channel, instrument.velocity, false);
}

/** Schedule melody note events for one measure */
void MusicMaker::scheduleMelody(const vector<shared_ptr<ScheduledEvent>>& schedule) {
	for (const auto& e: schedule) {
		if (isNoActualNote(e->note))
			continue;

		// Shorten duration if intensity is high
		const double duration = musicState.leadStyle == "Pulse" ? e->duration * 0.5 : e->duration;

		// Schedule base note
		scheduleNote(e->note, e->startTime, duration, LEAD);

		// Add a pause after short notes if staccato
		if (musicState.leadStyle == "Pulse")
			scheduleNote(PAUSE, e->startTime + doubleToMs(duration), duration, LEAD);

		// Schedule melody layers
		for (int i = 2; e->note + (i - 1) * 12 < 128; ++i)
			if (musicState.leadLayers >= i)
				scheduleNote(e->note + (i - 1) * 12, e->startTime, duration, LEAD);
	}
}

/** Schedule bass notes for one measure */
void MusicMaker::scheduleBass(const Chord& nextChord, const Clock::time_point mstAbs) {
	const Note root = nextChord.root;

	if (musicState.bassStyle == "Sustain") {
		const Note bassNote = root == 0 ? root + 36 : root + 24; // higher bass
		scheduleNote(bassNote, mstAbs, tsInfo.msPerMeas, BASS);

	} else {
		const Note bassNote = root == 11 ? root + 12 : root + 24; // lower bass

		if (musicState.bassStyle == "Pulse") {
			const double pulseDuration = tsInfo.msPerBeat * 0.6;

			for (int i = 0; i < tsInfo.num; ++i) {
				const auto pulseStart= mstAbs + doubleToMs(i * tsInfo.msPerBeat);
				scheduleNote(bassNote, pulseStart, pulseDuration, BASS);
			}
		} else if (musicState.bassStyle == "Fast") {
			const double pulseDuration = tsInfo.msPerBeat * 0.15;

			for (int i = 0; i < tsInfo.num * 4; ++i) {
				const auto pulseStart= mstAbs + doubleToMs(static_cast<double>(i) * 0.25 * tsInfo.msPerBeat);
				scheduleNote(
					(i - 2) % 4 == 0 ? bassNote + 12 : bassNote,  // octave up on every offbeat
					pulseStart,
					pulseDuration,
					BASS
				);
			}
		}
	}
}

/** Schedule drum groove for one measure */
void MusicMaker::scheduleDrums(const Clock::time_point mstAbs) {
	if (const auto it = drumPatterns.find(musicState.drumPattern); it != drumPatterns.end()) {
		for (const auto& [note, beatOffsets]: it->second) {
			for (const double i: beatOffsets) {
				const auto pulseStart= mstAbs + doubleToMs(i * tsInfo.msPerBeat);
				scheduleNote(note, pulseStart, 0, DRUMS);
			}
		}
	} else {
		cerr << "[MusicMaker] Unknown drum_pattern: " << static_cast<int>(musicState.drumPattern) << endl;
	}
}


/**
 * When changing chords, make sure that only new notes are being triggered,
 * and common notes between old and new chord are simply held through
 */
void MusicMaker::playChordTransition(const Chord& lastChord, const Chord& nextChord) {
	vector<Note> lastChordNotes;
	for (const int interval : lastChord.type.intervals)
		lastChordNotes.emplace_back((lastChord.root + interval) % 12);

	vector<Note> nextChordNotes;
	for (const int interval : nextChord.type.intervals)
		nextChordNotes.emplace_back((nextChord.root + interval) % 12);

	// If the game was just resumed from pause, force chord retriggering
	const bool forceRetrigger = wasJustResumed.exchange(false);

	// Stop notes no longer in the chord
	for (const auto& note : lastChordNotes) {
		if (forceRetrigger || !ranges::contains(nextChordNotes, note))
			midi.stopNote(note + 60, CHORDS.channel);

		// Always stop upper layers
		for (int i = 2; note + (i + 4) * 12 < 128; ++i)
			midi.stopNote(note + (i + 4) * 12, CHORDS.channel);
	}

	// Play new notes or retrigger on resume
	for (const auto& note : nextChordNotes) {
		if (forceRetrigger || !ranges::contains(lastChordNotes, note))
			midi.playNote(note + 60, CHORDS.channel, CHORDS.velocity);

		// Play chord layers
		for (int i = 2; note + (i + 4) * 12 < 128; ++i)
			if (musicState.chordLayers >= i)
				midi.playNote(note + (i + 4) * 12, CHORDS.channel, CHORDS.velocity);
	}
}


void MusicMaker::pause() {
	if (isPaused) return;
	isPaused = true;

	cout << "[MusicMaker] Game paused. Stopping music...\n";
	midi.stopAll();

	pauseTime = Clock::now();
}

void MusicMaker::resume() {
	// Compensate for the pause duration
	const auto resumeTime = Clock::now();
	const auto pauseDuration = resumeTime - pauseTime;
	playStartTime += pauseDuration;

	wasJustResumed = true;

	cout << "[MusicMaker] Game resumed.\n";
}
