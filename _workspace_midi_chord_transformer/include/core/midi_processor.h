#pragma once

#include "midi_structures.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

namespace midi_transformer {

// Forward declarations
class ChordProgressionAnalyzer;
class VoiceLeadingEngine;
class KeyDetector;
class ChordSynthesizer;
class ActionManager;

// Chord Detection Cache for performance optimization
struct ChordDetectionCache {
    std::string midiFileHash;
    std::vector<std::shared_ptr<Chord>> detectedChords;
    std::chrono::system_clock::time_point timestamp;
};

class MidiProcessor {
private:
    // Core MIDI data
    std::unique_ptr<MidiFile> midiFile;
    std::vector<Note> notes;
    std::vector<std::shared_ptr<Chord>> chords;
    uint32_t timeTolerance;
    std::string currentFilename;
    
    // Enhanced components using smart pointers
    std::unique_ptr<ChordProgressionAnalyzer> progressionAnalyzer;
    std::unique_ptr<VoiceLeadingEngine> voiceLeadingEngine;
    std::unique_ptr<KeyDetector> keyDetector;
    std::unique_ptr<ChordSynthesizer> synthesizer;
    std::shared_ptr<ActionManager> actionManager;
    
    // Cache for performance optimization
    std::unordered_map<std::string, std::shared_ptr<ChordDetectionCache>> detectionCache;
    
    // Utility functions for MIDI file I/O
    uint32_t readVariableLength(const std::vector<uint8_t>& data, size_t& position);
    void writeVariableLength(std::vector<uint8_t>& data, uint32_t value);
    uint16_t read16BE(const std::vector<uint8_t>& data, size_t position);
    void write16BE(std::vector<uint8_t>& data, uint16_t value);
    uint32_t read32BE(const std::vector<uint8_t>& data, size_t position);
    void write32BE(std::vector<uint8_t>& data, uint32_t value);
    
    // Chord detection and analysis
    void extractNotes();
    void detectChords();
    std::vector<int> normalizeChord(const std::vector<uint8_t>& notes);
    std::string identifyChord(const std::vector<uint8_t>& notes);
    std::string formatNotes(const std::vector<uint8_t>& notes);
    std::pair<std::string, std::string> parseChordName(const std::string& chordName);
    
    // Chord transformation
    std::vector<uint8_t> transformChord(
        const std::vector<uint8_t>& notes,
        const std::string& targetChordName,
        const TransformationOptions& options);
    
    // File hash calculation for caching
    std::string calculateFileHash(const std::string& filename);
    
public:
    MidiProcessor();
    
    // Prevent copying but allow moving
    MidiProcessor(const MidiProcessor&) = delete;
    MidiProcessor& operator=(const MidiProcessor&) = delete;
    MidiProcessor(MidiProcessor&&) = default;
    MidiProcessor& operator=(MidiProcessor&&) = default;
    
    // Destructor
    ~MidiProcessor() = default;
    
    // MIDI file operations
    bool loadMidiFile(const std::string& filename);
    bool writeMidiFile(const std::string& filename);
    
    // Chord operations
    std::vector<std::shared_ptr<Chord>> getChords() const;
    std::shared_ptr<Chord> getChord(size_t index) const;
    bool updateChord(size_t index, const Chord& newChordData);
    
    // Transformation operations
    void transformSelectedChords(
        const std::vector<int>& selectedIndices,
        const std::vector<std::string>& targetChordNames,
        const std::vector<std::shared_ptr<TransformationOptions>>& options);
    
    void switchTonality(size_t chordIndex);
    
    // Utility functions
    void setTimeTolerance(uint32_t tolerance);
    uint32_t getTimeTolerance() const;
    std::string getCurrentFilename() const;
    void displayChords() const;
    void displayTransformedChords() const;
    bool saveChordAnalysis(const std::string& filename) const;
    
    // Advanced features
    void analyzeProgression();
    void detectKey();
    void previewChord(size_t index);
    bool undo();
    bool redo();
};

} // namespace midi_transformer