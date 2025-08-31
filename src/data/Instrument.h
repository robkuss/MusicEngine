#pragma once

#include "util/Util.h"


namespace Instrument {
	struct GM_Instrument {
		int program;  // General MIDI program number (0-127)
	};

	constexpr GM_Instrument GRAND_PIANO		   = {};  // 0 initializer is redundant
	constexpr GM_Instrument BRIGHT_PIANO	   = { 1 };
	constexpr GM_Instrument E_GRAND_PIANO	   = { 2 };
	constexpr GM_Instrument HONKY_TONK		   = { 3 };
	constexpr GM_Instrument E_PIANO_1		   = { 4 };
	constexpr GM_Instrument E_PIANO_2		   = { 5 };
	constexpr GM_Instrument HARPSICHORD        = { 6 };
	constexpr GM_Instrument CLAVI              = { 7 };
	constexpr GM_Instrument CELESTA            = { 8 };
	constexpr GM_Instrument GLOCKENSPIEL       = { 9 };
	constexpr GM_Instrument MUSIC_BOX          = { 10 };
	constexpr GM_Instrument VIBRAPHONE         = { 11 };
	constexpr GM_Instrument MARIMBA            = { 12 };
	constexpr GM_Instrument XYLOPHONE          = { 13 };
	constexpr GM_Instrument TUBULAR_BELLS      = { 14 };
	constexpr GM_Instrument DULCIMER           = { 15 };
	constexpr GM_Instrument DRAWBAR_ORGAN      = { 16 };
	constexpr GM_Instrument PERCUSSIVE_ORGAN   = { 17 };
	constexpr GM_Instrument ROCK_ORGAN         = { 18 };
	constexpr GM_Instrument CHURCH_ORGAN       = { 19 };
	constexpr GM_Instrument REED_ORGAN         = { 20 };
	constexpr GM_Instrument ACCORDION          = { 21 };
	constexpr GM_Instrument HARMONICA          = { 22 };
	constexpr GM_Instrument TANGO_ACCORDION    = { 23 };
	constexpr GM_Instrument NYLON_GUITAR	   = { 24 };
	constexpr GM_Instrument STEEL_GUITAR	   = { 25 };
	constexpr GM_Instrument JAZZ_GUITAR		   = { 26 };
	constexpr GM_Instrument CLEAN_GUITAR	   = { 27 };
	constexpr GM_Instrument MUTED_GUITAR	   = { 28 };
	constexpr GM_Instrument OVERDRIVE_GUITAR   = { 29 };
	constexpr GM_Instrument DISTORTION_GUITAR  = { 30 };
	constexpr GM_Instrument GUITAR_HARMONICS   = { 31 };
	constexpr GM_Instrument ACOUSTIC_BASS      = { 32 };
	constexpr GM_Instrument FINGERED_BASS	   = { 33 };
	constexpr GM_Instrument PICKED_BASS		   = { 34 };
	constexpr GM_Instrument FRETLESS_BASS      = { 35 };
	constexpr GM_Instrument SLAP_BASS_1        = { 36 };
	constexpr GM_Instrument SLAP_BASS_2        = { 37 };
	constexpr GM_Instrument SYNTH_BASS_1       = { 38 };
	constexpr GM_Instrument SYNTH_BASS_2       = { 39 };
	constexpr GM_Instrument VIOLIN             = { 40 };
	constexpr GM_Instrument VIOLA              = { 41 };
	constexpr GM_Instrument CELLO              = { 42 };
	constexpr GM_Instrument CONTRABASS         = { 43 };
	constexpr GM_Instrument TREMOLO_STRINGS    = { 44 };
	constexpr GM_Instrument PIZZICATO_STRINGS  = { 45 };
	constexpr GM_Instrument ORCHESTRAL_HARP    = { 46 };
	constexpr GM_Instrument TIMPANI            = { 47 };
	constexpr GM_Instrument STRING_ENSEMBLE_1  = { 48 };
	constexpr GM_Instrument STRING_ENSEMBLE_2  = { 49 };
	constexpr GM_Instrument SYNTHSTRINGS_1     = { 50 };
	constexpr GM_Instrument SYNTHSTRINGS_2     = { 51 };
	constexpr GM_Instrument CHOIR_AAHS         = { 52 };
	constexpr GM_Instrument VOICE_OOHS         = { 53 };
	constexpr GM_Instrument SYNTH_VOICE        = { 54 };
	constexpr GM_Instrument ORCHESTRA_HIT      = { 55 };
	constexpr GM_Instrument TRUMPET            = { 56 };
	constexpr GM_Instrument TROMBONE           = { 57 };
	constexpr GM_Instrument TUBA               = { 58 };
	constexpr GM_Instrument MUTED_TRUMPET      = { 59 };
	constexpr GM_Instrument FRENCH_HORN        = { 60 };
	constexpr GM_Instrument BRASS_SECTION      = { 61 };
	constexpr GM_Instrument SYNTHBRASS_1       = { 62 };
	constexpr GM_Instrument SYNTHBRASS_2       = { 63 };
	constexpr GM_Instrument SOPRANO_SAX        = { 64 };
	constexpr GM_Instrument ALTO_SAX           = { 65 };
	constexpr GM_Instrument TENOR_SAX          = { 66 };
	constexpr GM_Instrument BARITONE_SAX       = { 67 };
	constexpr GM_Instrument OBOE               = { 68 };
	constexpr GM_Instrument ENGLISH_HORN       = { 69 };
	constexpr GM_Instrument BASSOON            = { 70 };
	constexpr GM_Instrument CLARINET           = { 71 };
	constexpr GM_Instrument PICCOLO            = { 72 };
	constexpr GM_Instrument FLUTE              = { 73 };
	constexpr GM_Instrument RECORDER           = { 74 };
	constexpr GM_Instrument PAN_FLUTE          = { 75 };
	constexpr GM_Instrument BLOWN_BOTTLE       = { 76 };
	constexpr GM_Instrument SHAKUHACHI         = { 77 };
	constexpr GM_Instrument WHISTLE            = { 78 };
	constexpr GM_Instrument OCARINA            = { 79 };
	constexpr GM_Instrument LEAD_1_SQUARE      = { 80 };
	constexpr GM_Instrument LEAD_2_SAWTOOTH    = { 81 };
	constexpr GM_Instrument LEAD_3_CALLIOPE    = { 82 };
	constexpr GM_Instrument LEAD_4_CHIFF       = { 83 };
	constexpr GM_Instrument LEAD_5_CHARANG     = { 84 };
	constexpr GM_Instrument LEAD_6_VOICE       = { 85 };
	constexpr GM_Instrument LEAD_7_FIFTHS      = { 86 };
	constexpr GM_Instrument LEAD_8_BASS_LEAD   = { 87 };
	constexpr GM_Instrument PAD_1_NEW_AGE      = { 88 };
	constexpr GM_Instrument PAD_2_WARM         = { 89 };
	constexpr GM_Instrument PAD_3_POLYSYNTH    = { 90 };
	constexpr GM_Instrument PAD_4_CHOIR        = { 91 };
	constexpr GM_Instrument PAD_5_BOWED        = { 92 };
	constexpr GM_Instrument PAD_6_METALLIC     = { 93 };
	constexpr GM_Instrument PAD_7_HALO         = { 94 };
	constexpr GM_Instrument PAD_8_SWEEP        = { 95 };
	constexpr GM_Instrument FX_1_RAIN          = { 96 };
	constexpr GM_Instrument FX_2_SOUNDTRACK    = { 97 };
	constexpr GM_Instrument FX_3_CRYSTAL       = { 98 };
	constexpr GM_Instrument FX_4_ATMOSPHERE    = { 99 };
	constexpr GM_Instrument FX_5_BRIGHTNESS    = { 100 };
	constexpr GM_Instrument FX_6_GOBLINS       = { 101 };
	constexpr GM_Instrument FX_7_ECHOES        = { 102 };
	constexpr GM_Instrument FX_8_SCI_FI        = { 103 };
	constexpr GM_Instrument SITAR			   = { 104 };
	constexpr GM_Instrument BANJO              = { 105 };
	constexpr GM_Instrument SHAMISEN           = { 106 };
	constexpr GM_Instrument KOTO               = { 107 };
	constexpr GM_Instrument KALIMBA            = { 108 };
	constexpr GM_Instrument BAG_PIPE           = { 109 };
	constexpr GM_Instrument FIDDLE             = { 110 };
	constexpr GM_Instrument SHANAI             = { 111 };
	constexpr GM_Instrument TINKLE_BELL        = { 112 };
	constexpr GM_Instrument AGOGO              = { 113 };
	constexpr GM_Instrument STEEL_DRUMS        = { 114 };
	constexpr GM_Instrument WOODBLOCK          = { 115 };
	constexpr GM_Instrument TAIKO_DRUM         = { 116 };
	constexpr GM_Instrument MELODIC_TOM        = { 117 };
	constexpr GM_Instrument SYNTH_DRUM         = { 118 };
	constexpr GM_Instrument REVERSE_CYMBAL     = { 119 };
	constexpr GM_Instrument GUITAR_FRET_NOISE  = { 120 };
	constexpr GM_Instrument BREATH_NOISE       = { 121 };
	constexpr GM_Instrument SEASHORE           = { 122 };
	constexpr GM_Instrument BIRD_TWEET         = { 123 };
	constexpr GM_Instrument TELEPHONE_RING     = { 124 };
	constexpr GM_Instrument HELICOPTER         = { 125 };
	constexpr GM_Instrument APPLAUSE           = { 126 };
	constexpr GM_Instrument GUNSHOT            = { 127 };

	// Drums
	constexpr Note BASS_DRUM_2	 = 35;
	constexpr Note BASS_DRUM_1	 = 36;
	constexpr Note SIDE_STICK	 = 37;
	constexpr Note SNARE_1		 = 38;
	constexpr Note HAND_CLAP	 = 39;
	constexpr Note SNARE_2		 = 40;
	constexpr Note LOW_TOM_2	 = 41;
	constexpr Note CLOSED_HAT	 = 42;
	constexpr Note LOW_TOM_1	 = 43;
	constexpr Note PEDAL_HAT	 = 44;
	constexpr Note MID_TOM_2	 = 45;
	constexpr Note OPEN_HAT		 = 46;
	constexpr Note MID_TOM_1	 = 47;
	constexpr Note HIGH_TOM_2	 = 48;
	constexpr Note CRASH_CYM_1	 = 49;
	constexpr Note HIGH_TOM_1	 = 50;
	constexpr Note RIDE_CYM_1	 = 51;
	constexpr Note CHINA_CYM	 = 52;
	constexpr Note RIDE_BELL	 = 53;
	constexpr Note TAMBOURINE	 = 54;
	constexpr Note SPLASH_CYM	 = 55;
	constexpr Note COWBELL		 = 56;
	constexpr Note CRASH_CYM_2	 = 57;
	constexpr Note VIBRASLAP     = 58;
	constexpr Note RIDE_CYM_2    = 59;
	constexpr Note HI_BONGO      = 60;
	constexpr Note LOW_BONGO     = 61;
	constexpr Note MUTE_HI_CONGA = 62;
	constexpr Note OPEN_HI_CONGA = 63;
	constexpr Note LOW_CONGA     = 64;
	constexpr Note HIGH_TIMBALE  = 65;
	constexpr Note LOW_TIMBALE   = 66;
	constexpr Note HIGH_AGOGO    = 67;
	constexpr Note LOW_AGOGO     = 68;
	constexpr Note CABASA        = 69;
	constexpr Note MARACAS       = 70;
	constexpr Note SHORT_WHISTLE = 71;
	constexpr Note LONG_WHISTLE  = 72;
	constexpr Note SHORT_GUIRO   = 73;
	constexpr Note LONG_GUIRO    = 74;
	constexpr Note CLAVES        = 75;
	constexpr Note WOODBLOCK_H	 = 76;
	constexpr Note WOODBLOCK_L	 = 77;
	constexpr Note MUTE_CUICA    = 78;
	constexpr Note OPEN_CUICA    = 79;
	constexpr Note MUTE_TRIANGLE = 80;
	constexpr Note OPEN_TRIANGLE = 81;

	// General MIDI Extensions (not in the original GM1 spec but supported in some soundfonts)
	constexpr Note SHAKE         = 82;
	constexpr Note JINGLE_BELL   = 83;
	constexpr Note BELL_TREE     = 84;
	constexpr Note CASTANETS     = 85;
	constexpr Note MUTE_SURDO    = 86;
	constexpr Note OPEN_SURDO    = 87;
}



struct ActiveInstrument {
	int channel;   // MIDI channel (0–15), dynamically assigned
	int program;   // General MIDI program number (0–127)
	int velocity;  // Default velocity (0–127) for this instrument
};

inline constexpr auto LEAD	 = ActiveInstrument( 0, Instrument::STRING_ENSEMBLE_1.program, 127);
inline constexpr auto CHORDS = ActiveInstrument( 1, Instrument::FX_7_ECHOES.program,	   127);
inline constexpr auto BASS	 = ActiveInstrument( 2, Instrument::FINGERED_BASS.program,     127);
inline constexpr auto DRUMS	 = ActiveInstrument( 9, 0,							   127);
