#include "MIDI.h"

using namespace std;
using namespace smf;

// #define LIVE_MIDI_TRACK	1


MIDI::MIDI() {
	// Initialize FluidSynth
	settings = new_fluid_settings();
	synth	 = new_fluid_synth(settings);
	adriver  = new_fluid_audio_driver(settings, synth);

	// RtMidi setup
	/*try {
		midiIn = new RtMidiIn();
	} catch (RtMidiError& error) {
		error.printMessage();
		exit(EXIT_FAILURE);
	}*/

	// List available MIDI ports
	/*const unsigned int nPorts = midiIn->getPortCount();
	if (nPorts == 0) {
		cout << "No MIDI input ports available." << endl;
		exit(EXIT_FAILURE);
	}
	cout << "Available MIDI input ports:" << endl;
	for (unsigned int i = 0; i < nPorts; i++) {
		cout << i << ": " << midiIn->getPortName(i) << endl;
	}*/
}

MIDI::~MIDI() {
	// Cleanup
	delete_fluid_audio_driver(adriver);
	delete_fluid_synth(synth);
	delete_fluid_settings(settings);
}


void MIDI::loadSoundfont(const string& soundfont) {
	sfid = fluid_synth_sfload(synth, (RESOURCES_DIR + soundfont).c_str(), 1);
}

void MIDI::selectProgram(const unsigned char channel, const int instrument) const {
	fluid_synth_program_select(synth, channel, sfid, 0, instrument);  // channel, sfid, bank, preset
}


void MIDI::playNote(const Note note, const unsigned char channel, const unsigned char velocity) const {
	fluid_synth_noteon(synth, channel, note, velocity);
}

void MIDI::stopNote(const Note note, const unsigned char channel) const {
	fluid_synth_noteoff(synth, channel, note);
}

/** All Notes Off */
void MIDI::stopAll() const {
	for (int channel = 0; channel < 16; ++channel) {
		for (Note note = 0; note < 128; ++note) {
			fluid_synth_noteoff(synth, channel, note);
		}
	}
}


// Live recording (unused)
/*void MIDI::prepareLiveMIDIFile() {
	// Prepare the SMF structure for a new MIDI file for live recording
	liveMidiFile.absoluteTicks();
	liveMidiFile.addTrack(LIVE_MIDI_TRACK);
	liveMidiFile.setTicksPerQuarterNote(480);

	// Open the first port (MIDI keyboard)
	midiIn->openPort(0);
}

vector<unsigned char> MIDI::getMessage() const {
	vector<unsigned char> msg;
	midiIn->getMessage(&msg);
	return msg;
}

void MIDI::finishRecording() const {
	if (midiIn->isPortOpen()) midiIn->closePort();  // Close live MIDI input port
}


void MIDI::addNoteOn(const int ticks, const Note note, const unsigned char channel, const unsigned char velocity) {
	liveMidiFile.addNoteOn(LIVE_MIDI_TRACK, ticks, channel, note, velocity);
}

void MIDI::addNoteOff(const int ticks, const Note note, const unsigned char channel, const unsigned char velocity) {
	liveMidiFile.addNoteOff(LIVE_MIDI_TRACK, ticks, channel, note, velocity);
}*/
