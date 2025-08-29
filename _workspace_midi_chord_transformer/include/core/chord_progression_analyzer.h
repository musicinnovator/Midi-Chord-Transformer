#pragma once

#include "midi_structures.h"
#include <vector>
#include <string>
#include <memory>

namespace midi_transformer {

// For storing common progression patterns
struct ProgressionPattern {
    std::vector<std::string> chordQualities;  // e.g., ["m7", "7", "maj7"] for ii-V-I
    std::string name;                         // Name of the progression
    std::vector<std::string> commonKeys;      // Keys where this progression is common
};

// For analyzing chord progressions and patterns
struct ChordProgression {
    std::vector<int> chordIndices;  // Indices of chords in the progression
    std::string progressionName;    // e.g., "ii-V-I", "12-bar blues"
    double confidence;              // Confidence level of the detection
};

class ChordProgressionAnalyzer {
private:
    std::vector<std::shared_ptr<ProgressionPattern>> knownPatterns;
    
    void loadPatterns();
    
public:
    ChordProgressionAnalyzer();
    
    std::vector<std::shared_ptr<ChordProgression>> detectProgressions(
        const std::vector<std::shared_ptr<Chord>>& chords);
    
    void addPattern(const ProgressionPattern& pattern);
    std::vector<std::shared_ptr<ProgressionPattern>> getKnownPatterns() const;
};

} // namespace midi_transformer