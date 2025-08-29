#include "../../include/utils/midi_utils.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <filesystem>
#include <unordered_map>
#include <cctype>

namespace midi_transformer {
namespace utils {

std::string generateTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
    return ss.str();
}

std::string getBaseFilename(const std::string& path) {
    // Extract filename without extension
    std::filesystem::path filePath(path);
    return filePath.stem().string();
}

std::vector<std::string> findMidiFiles(const std::string& directory) {
    std::vector<std::string> midiFiles;
    
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            std::string extension = entry.path().extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
            
            if (extension == ".mid" || extension == ".midi") {
                midiFiles.push_back(entry.path().string());
            }
        }
    }
    
    return midiFiles;
}

std::string getFileExtension(const std::string& filename) {
    std::filesystem::path filePath(filename);
    std::string extension = filePath.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    return extension;
}

bool createDirectory(const std::string& path) {
    try {
        return std::filesystem::create_directories(path);
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error creating directory: " << e.what() << std::endl;
        return false;
    }
}

std::string midiNoteToName(uint8_t noteNumber) {
    static const std::vector<std::string> noteNames = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    
    int octave = noteNumber / 12 - 1;
    int noteIndex = noteNumber % 12;
    
    return noteNames[noteIndex] + std::to_string(octave);
}

uint8_t noteNameToMidi(const std::string& noteName) {
    static const std::unordered_map<std::string, int> noteToIndex = {
        {"C", 0}, {"C#", 1}, {"Db", 1}, {"D", 2}, {"D#", 3}, {"Eb", 3},
        {"E", 4}, {"F", 5}, {"F#", 6}, {"Gb", 6}, {"G", 7}, {"G#", 8},
        {"Ab", 8}, {"A", 9}, {"A#", 10}, {"Bb", 10}, {"B", 11}
    };
    
    // Extract note and octave
    std::string note;
    int octave = 4; // Default octave if not specified
    
    if (noteName.size() >= 2 && std::isdigit(noteName.back())) {
        // Note with octave (e.g., "C4")
        octave = noteName.back() - '0';
        note = noteName.substr(0, noteName.size() - 1);
    } else {
        // Note without octave (e.g., "C")
        note = noteName;
    }
    
    // Look up note index
    auto it = noteToIndex.find(note);
    if (it == noteToIndex.end()) {
        std::cerr << "Error: Invalid note name: " << note << std::endl;
        return 60; // Default to middle C
    }
    
    // Calculate MIDI note number
    return static_cast<uint8_t>((octave + 1) * 12 + it->second);
}

std::string formatDuration(uint32_t ticks, uint16_t division) {
    // Convert ticks to beats
    double beats = static_cast<double>(ticks) / division;
    
    // Format as "X.XX beats"
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << beats << " beats";
    return ss.str();
}

std::string formatChordNotes(const std::vector<uint8_t>& notes) {
    std::stringstream ss;
    for (size_t i = 0; i < notes.size(); i++) {
        ss << midiNoteToName(notes[i]);
        if (i < notes.size() - 1) {
            ss << ", ";
        }
    }
    return ss.str();
}

int getIntervalBetweenNotes(uint8_t note1, uint8_t note2) {
    return std::abs(static_cast<int>(note1) - static_cast<int>(note2));
}

std::vector<int> getChordIntervals(const std::vector<uint8_t>& notes) {
    if (notes.empty()) {
        return {};
    }
    
    // Find the lowest note
    uint8_t lowestNote = *std::min_element(notes.begin(), notes.end());
    
    // Calculate intervals relative to the lowest note
    std::vector<int> intervals;
    for (uint8_t note : notes) {
        intervals.push_back(note - lowestNote);
    }
    
    // Sort intervals
    std::sort(intervals.begin(), intervals.end());
    
    return intervals;
}

std::string formatChordName(const std::string& root, const std::string& quality) {
    return root + quality;
}

std::pair<std::string, std::string> parseChordName(const std::string& chordName) {
    static const std::vector<std::string> noteNames = {
        "C", "C#", "Db", "D", "D#", "Eb", "E", "F", "F#", "Gb", "G", "G#", "Ab", "A", "A#", "Bb", "B"
    };
    
    // Find the root note
    std::string rootNote;
    size_t pos = 0;
    
    // Check for note names (longest first to handle flats/sharps)
    for (const auto& noteName : noteNames) {
        if (chordName.substr(0, noteName.size()) == noteName) {
            rootNote = noteName;
            pos = noteName.size();
            break;
        }
    }
    
    // If no valid root note found, default to C
    if (rootNote.empty()) {
        rootNote = "C";
        pos = 0;
    }
    
    // Check for bass note (e.g., C/E)
    size_t slashPos = chordName.find('/');
    std::string quality;
    
    if (slashPos != std::string::npos) {
        quality = chordName.substr(pos, slashPos - pos);
    } else {
        quality = chordName.substr(pos);
    }
    
    return {rootNote, quality};
}

std::string getChordRoot(const std::string& chordName) {
    return parseChordName(chordName).first;
}

std::string getChordQuality(const std::string& chordName) {
    return parseChordName(chordName).second;
}

std::vector<uint8_t> getChordNotesFromName(const std::string& chordName, uint8_t baseOctave) {
    // Parse the chord name
    auto [rootNote, quality] = parseChordName(chordName);
    
    // Convert root note to MIDI note number
    uint8_t rootMidiNote = noteNameToMidi(rootNote);
    
    // Adjust to the specified octave
    rootMidiNote = (rootMidiNote % 12) + (baseOctave * 12);
    
    // Define chord intervals based on quality
    std::vector<int> intervals;
    
    // Major and minor triads
    if (quality.empty()) {
        intervals = {0, 4, 7}; // Major triad
    } else if (quality == "m") {
        intervals = {0, 3, 7}; // Minor triad
    }
    // Seventh chords
    else if (quality == "7") {
        intervals = {0, 4, 7, 10}; // Dominant 7th
    } else if (quality == "maj7") {
        intervals = {0, 4, 7, 11}; // Major 7th
    } else if (quality == "m7") {
        intervals = {0, 3, 7, 10}; // Minor 7th
    } else if (quality == "dim7") {
        intervals = {0, 3, 6, 9}; // Diminished 7th
    } else if (quality == "m7b5" || quality == "ø") {
        intervals = {0, 3, 6, 10}; // Half-diminished 7th
    }
    // Extended chords
    else if (quality == "9") {
        intervals = {0, 4, 7, 10, 14}; // Dominant 9th
    } else if (quality == "maj9") {
        intervals = {0, 4, 7, 11, 14}; // Major 9th
    } else if (quality == "m9") {
        intervals = {0, 3, 7, 10, 14}; // Minor 9th
    }
    // Sixth chords
    else if (quality == "6") {
        intervals = {0, 4, 7, 9}; // Major 6th
    } else if (quality == "m6") {
        intervals = {0, 3, 7, 9}; // Minor 6th
    }
    // Suspended chords
    else if (quality == "sus4") {
        intervals = {0, 5, 7}; // Suspended 4th
    } else if (quality == "sus2") {
        intervals = {0, 2, 7}; // Suspended 2nd
    } else if (quality == "7sus4") {
        intervals = {0, 5, 7, 10}; // 7th suspended 4th
    }
    // Augmented and diminished
    else if (quality == "aug") {
        intervals = {0, 4, 8}; // Augmented
    } else if (quality == "dim") {
        intervals = {0, 3, 6}; // Diminished
    }
    // Add chords
    else if (quality == "add9") {
        intervals = {0, 4, 7, 14}; // Add 9
    } else if (quality == "madd9") {
        intervals = {0, 3, 7, 14}; // Minor add 9
    }
    // Default to major triad if quality not recognized
    else {
        intervals = {0, 4, 7};
    }
    
    // Create the chord notes
    std::vector<uint8_t> notes;
    for (int interval : intervals) {
        uint8_t note = rootMidiNote + interval;
        if (note <= 127) { // Ensure note is within MIDI range
            notes.push_back(note);
        }
    }
    
    // Handle slash chords (e.g., C/E)
    size_t slashPos = chordName.find('/');
    if (slashPos != std::string::npos && slashPos + 1 < chordName.size()) {
        std::string bassNoteName = chordName.substr(slashPos + 1);
        uint8_t bassNote = noteNameToMidi(bassNoteName);
        
        // Adjust to the specified octave
        bassNote = (bassNote % 12) + ((baseOctave - 1) * 12);
        
        // Add bass note if it's not already in the chord
        if (std::find(notes.begin(), notes.end(), bassNote) == notes.end()) {
            notes.insert(notes.begin(), bassNote);
        }
    }
    
    return notes;
}

std::string calculateFileHash(const std::string& filename) {
    // Simple hash function for demonstration
    // In a real implementation, use a proper hash algorithm like MD5 or SHA-1
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    std::stringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();
    
    size_t hash = 0;
    for (char c : content) {
        hash = hash * 31 + c;
    }
    
    std::stringstream hashStr;
    hashStr << std::hex << std::setw(16) << std::setfill('0') << hash;
    return hashStr.str();
}

std::string calculateDataHash(const std::vector<uint8_t>& data) {
    // Simple hash function for demonstration
    // In a real implementation, use a proper hash algorithm like MD5 or SHA-1
    size_t hash = 0;
    for (uint8_t byte : data) {
        hash = hash * 31 + byte;
    }
    
    std::stringstream hashStr;
    hashStr << std::hex << std::setw(16) << std::setfill('0') << hash;
    return hashStr.str();
}

} // namespace utils
} // namespace midi_transformer