#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace midi_transformer {
namespace utils {

// File operations
std::string generateTimestamp();
std::string getBaseFilename(const std::string& path);
std::vector<std::string> findMidiFiles(const std::string& directory);
std::string getFileExtension(const std::string& filename);
bool createDirectory(const std::string& path);

// MIDI-specific utilities
std::string midiNoteToName(uint8_t noteNumber);
uint8_t noteNameToMidi(const std::string& noteName);
std::string formatDuration(uint32_t ticks, uint16_t division);
std::string formatChordNotes(const std::vector<uint8_t>& notes);
int getIntervalBetweenNotes(uint8_t note1, uint8_t note2);
std::vector<int> getChordIntervals(const std::vector<uint8_t>& notes);

// Chord name parsing and formatting
std::string formatChordName(const std::string& root, const std::string& quality);
std::pair<std::string, std::string> parseChordName(const std::string& chordName);
std::string getChordRoot(const std::string& chordName);
std::string getChordQuality(const std::string& chordName);
std::vector<uint8_t> getChordNotesFromName(const std::string& chordName, uint8_t baseOctave = 4);

// Hash calculation for caching
std::string calculateFileHash(const std::string& filename);
std::string calculateDataHash(const std::vector<uint8_t>& data);

} // namespace utils
} // namespace midi_transformer