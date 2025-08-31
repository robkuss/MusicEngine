#pragma once

#include <fluidsynth.h>
#include <RtMidi.h>
// #include <MidiFile.h>


class MIDI {
public:
	MIDI();
	void selectProgram(unsigned char channel, int instrument) const;
	~MIDI();
	void loadSoundfont(const std::string& soundfont);

	void playNote(Note note, unsigned char channel, unsigned char velocity) const;
	void stopNote(Note note, unsigned char channel) const;

	void stopAll() const;


	// Live recording (unused)
	// void prepareLiveMIDIFile();
	// smf::MidiFile& getLiveMIDIFile() { return liveMidiFile; }

	// [[nodiscard]] std::vector<unsigned char> getMessage() const;
	// void finishRecording() const;

	// void addNoteOn (int ticks, Note note, unsigned char channel, unsigned char velocity);
	// void addNoteOff(int ticks, Note note, unsigned char channel, unsigned char velocity);

private:
	fluid_settings_t* settings	  = nullptr;
	fluid_synth_t* synth		  = nullptr;
	fluid_audio_driver_t *adriver = nullptr;

	int sfid{};

	// RtMidiIn* midiIn;
	// smf::MidiFile liveMidiFile;
};
