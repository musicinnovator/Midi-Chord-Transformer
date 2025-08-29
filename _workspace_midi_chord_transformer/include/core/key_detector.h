#pragma once

#include "midi_structures.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <map>

namespace midi_transformer {

// For key detection and scale-aware transformations
struct KeySignature {
    std::string rootNote;           // Root note of the key
    bool isMajor;                   // Major or minor
    std::vector<uint8_t> scaleDegrees; // MIDI note values for scale degrees
    std::map<int, std::string> diatonicChords; // Diatonic chords for this key
};

// For scale-aware chord substitutions
struct ScaleConstraint {
    std::string scaleType;          // e.g., "major", "minor", "dorian"
    uint8_t rootNote;               // Root note of the scale
    std::vector<uint8_t> allowedNotes; // Notes that are in the scale
    std::vector<std::string> allowedChords; // Chords that fit the scale
};

class KeyDetector {
private:
    std::unordered_map<std::string, std::shared_ptr<KeySignature>> keySignatures;
    
    void initializeKeySignatures();
    int countNotesInKey(const std::vector<uint8_t>& notes, const std::shared_ptr<KeySignature>& key);
    
public:
    KeyDetector();
    
    std::shared_ptr<KeySignature> detectKey(const std::vector<std::shared_ptr<Chord>>& chords);
    
    std::vector<std::shared_ptr<ScaleConstraint>> getScaleConstraints(
        const std::shared_ptr<KeySignature>& key);
    
    std::shared_ptr<KeySignature> getKeySignature(const std::string& keyName);
    
    std::vector<std::string> getAllKeyNames() const;
};

} // namespace midi_transformer