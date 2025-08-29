#pragma once

#include "midi_structures.h"
#include <vector>
#include <memory>
#include <string>

namespace midi_transformer {

// For more sophisticated voice leading
struct VoiceLeadingOptions {
    bool minimizeMovement;          // Prioritize smallest possible movement
    bool avoidParallels;            // Avoid parallel fifths/octaves
    bool maintainVoiceCount;        // Keep the same number of voices
    int maxVoiceMovement;           // Maximum semitones a voice can move
    std::vector<int> voicePriority; // Which voices to prioritize in movement
    
    VoiceLeadingOptions() 
        : minimizeMovement(true), 
          avoidParallels(true), 
          maintainVoiceCount(true), 
          maxVoiceMovement(7) {}
};

// For tracking voice movement during transformations
struct VoiceMovement {
    uint8_t originalPitch;
    uint8_t newPitch;
    int movement;                   // Semitones moved
    bool isSmallestPossibleMove;    // Whether this is optimal movement
};

class VoiceLeadingEngine {
private:
    std::shared_ptr<VoiceLeadingOptions> options;
    
    // Helper methods for voice leading
    std::vector<uint8_t> findOptimalVoicing(
        const std::vector<uint8_t>& targetPitches,
        const std::vector<uint8_t>& originalNotes);
    
    bool hasParallelFifthsOrOctaves(
        const std::vector<uint8_t>& originalNotes,
        const std::vector<uint8_t>& newNotes);
    
    int calculateMovementCost(
        const std::vector<uint8_t>& originalNotes,
        const std::vector<uint8_t>& newNotes);
    
public:
    VoiceLeadingEngine(const VoiceLeadingOptions& opts);
    
    void setOptions(const VoiceLeadingOptions& opts);
    VoiceLeadingOptions getOptions() const;
    
    std::vector<uint8_t> transformChord(
        const std::vector<uint8_t>& originalNotes,
        const std::string& targetChordName,
        const TransformationOptions& transformOptions);
    
    std::vector<std::shared_ptr<VoiceMovement>> analyzeVoiceMovement(
        const std::vector<uint8_t>& originalNotes,
        const std::vector<uint8_t>& newNotes);
};

} // namespace midi_transformer