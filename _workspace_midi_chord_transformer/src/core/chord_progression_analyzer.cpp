#include "../../include/core/chord_progression_analyzer.h"
#include "../../include/utils/midi_utils.h"

#include <algorithm>
#include <unordered_map>
#include <iostream>

namespace midi_transformer {

ChordProgressionAnalyzer::ChordProgressionAnalyzer() {
    loadPatterns();
}

void ChordProgressionAnalyzer::loadPatterns() {
    // Load common chord progression patterns
    
    // ii-V-I (Jazz)
    auto iiVI = std::make_shared<ProgressionPattern>();
    iiVI->chordQualities = {"m7", "7", "maj7"};
    iiVI->name = "ii-V-I";
    iiVI->commonKeys = {"C", "F", "Bb", "Eb", "G", "D", "A"};
    knownPatterns.push_back(iiVI);
    
    // I-IV-V (Pop/Rock)
    auto IIV = std::make_shared<ProgressionPattern>();
    IIV->chordQualities = {"", "", ""};
    IIV->name = "I-IV-V";
    IIV->commonKeys = {"C", "G", "D", "A", "E", "F"};
    knownPatterns.push_back(IIV);
    
    // I-V-vi-IV (Pop)
    auto IVviIV = std::make_shared<ProgressionPattern>();
    IVviIV->chordQualities = {"", "", "m", ""};
    IVviIV->name = "I-V-vi-IV";
    IVviIV->commonKeys = {"C", "G", "D", "A", "F"};
    knownPatterns.push_back(IVviIV);
    
    // I-vi-IV-V (50s Progression)
    auto IviIVV = std::make_shared<ProgressionPattern>();
    IviIVV->chordQualities = {"", "m", "", ""};
    IviIVV->name = "I-vi-IV-V (50s)";
    IviIVV->commonKeys = {"C", "G", "D", "A", "F"};
    knownPatterns.push_back(IviIVV);
    
    // vi-IV-I-V (Pop)
    auto viIIV = std::make_shared<ProgressionPattern>();
    viIIV->chordQualities = {"m", "", "", ""};
    viIIV->name = "vi-IV-I-V";
    viIIV->commonKeys = {"C", "G", "D", "A", "F"};
    knownPatterns.push_back(viIIV);
    
    // I-V-vi-iii-IV-I-IV-V (Canon)
    auto canon = std::make_shared<ProgressionPattern>();
    canon->chordQualities = {"", "", "m", "m", "", "", "", ""};
    canon->name = "Canon Progression";
    canon->commonKeys = {"D", "G", "C"};
    knownPatterns.push_back(canon);
    
    // i-bVII-bVI-V (Andalusian Cadence)
    auto andalusian = std::make_shared<ProgressionPattern>();
    andalusian->chordQualities = {"m", "", "", ""};
    andalusian->name = "Andalusian Cadence";
    andalusian->commonKeys = {"Am", "Em", "Dm"};
    knownPatterns.push_back(andalusian);
    
    // I-bVII-IV (Mixolydian Vamp)
    auto mixolydian = std::make_shared<ProgressionPattern>();
    mixolydian->chordQualities = {"", "", ""};
    mixolydian->name = "Mixolydian Vamp";
    mixolydian->commonKeys = {"G", "D", "A", "E"};
    knownPatterns.push_back(mixolydian);
    
    // i-iv-v (Minor Blues)
    auto minorBlues = std::make_shared<ProgressionPattern>();
    minorBlues->chordQualities = {"m", "m", "m"};
    minorBlues->name = "Minor Blues";
    minorBlues->commonKeys = {"Am", "Em", "Dm", "Gm"};
    knownPatterns.push_back(minorBlues);
    
    // I-I7-IV-iv (Major-Minor Change)
    auto majorMinor = std::make_shared<ProgressionPattern>();
    majorMinor->chordQualities = {"", "7", "", "m"};
    majorMinor->name = "Major-Minor Change";
    majorMinor->commonKeys = {"C", "G", "D", "F"};
    knownPatterns.push_back(majorMinor);
}

std::vector<std::shared_ptr<ChordProgression>> ChordProgressionAnalyzer::detectProgressions(
    const std::vector<std::shared_ptr<Chord>>& chords) {
    
    std::vector<std::shared_ptr<ChordProgression>> results;
    
    if (chords.size() < 2) {
        return results; // Need at least 2 chords for a progression
    }
    
    // Extract chord qualities and roots
    std::vector<std::pair<std::string, std::string>> chordParts;
    for (const auto& chord : chords) {
        chordParts.push_back(utils::parseChordName(chord->name));
    }
    
    // Try to detect each known pattern
    for (const auto& pattern : knownPatterns) {
        // Skip if the pattern is longer than the chord sequence
        if (pattern->chordQualities.size() > chords.size()) {
            continue;
        }
        
        // Slide a window of pattern->chordQualities.size() through the chord sequence
        for (size_t startIdx = 0; startIdx <= chords.size() - pattern->chordQualities.size(); startIdx++) {
            double matchScore = 0.0;
            bool potentialMatch = true;
            
            // Check if the qualities match
            for (size_t i = 0; i < pattern->chordQualities.size(); i++) {
                size_t chordIdx = startIdx + i;
                
                // Extract just the basic quality (ignoring extensions)
                std::string chordQuality = chordParts[chordIdx].second;
                std::string patternQuality = pattern->chordQualities[i];
                
                // Basic quality match (e.g., "m7" matches "m")
                if (chordQuality.find(patternQuality) == 0 || 
                    (patternQuality.empty() && (chordQuality.empty() || chordQuality == "maj7" || chordQuality == "6" || chordQuality == "9"))) {
                    matchScore += 1.0;
                } 
                // Partial match (e.g., "m" is similar to "m7")
                else if (!patternQuality.empty() && !chordQuality.empty() && 
                         chordQuality[0] == patternQuality[0]) {
                    matchScore += 0.5;
                } else {
                    potentialMatch = false;
                    break;
                }
            }
            
            if (potentialMatch) {
                // Calculate confidence based on match score
                double confidence = matchScore / pattern->chordQualities.size();
                
                // Check if the root notes form a sensible key
                // This is a simplified approach - a real implementation would be more sophisticated
                std::string possibleKey = chordParts[startIdx].first;
                
                // Adjust confidence based on whether this key is common for this progression
                bool keyMatch = false;
                for (const auto& key : pattern->commonKeys) {
                    if (key == possibleKey || key == possibleKey + "m") {
                        keyMatch = true;
                        break;
                    }
                }
                
                if (keyMatch) {
                    confidence *= 1.2; // Boost confidence for common keys
                } else {
                    confidence *= 0.8; // Reduce confidence for uncommon keys
                }
                
                // If confidence is high enough, add to results
                if (confidence >= 0.6) {
                    auto progression = std::make_shared<ChordProgression>();
                    progression->progressionName = pattern->name + " in " + possibleKey;
                    progression->confidence = confidence;
                    
                    // Add chord indices
                    for (size_t i = 0; i < pattern->chordQualities.size(); i++) {
                        progression->chordIndices.push_back(startIdx + i);
                    }
                    
                    results.push_back(progression);
                }
            }
        }
    }
    
    // Sort results by confidence (highest first)
    std::sort(results.begin(), results.end(), 
              [](const std::shared_ptr<ChordProgression>& a, const std::shared_ptr<ChordProgression>& b) {
                  return a->confidence > b->confidence;
              });
    
    return results;
}

void ChordProgressionAnalyzer::addPattern(const ProgressionPattern& pattern) {
    auto newPattern = std::make_shared<ProgressionPattern>(pattern);
    knownPatterns.push_back(newPattern);
}

std::vector<std::shared_ptr<ProgressionPattern>> ChordProgressionAnalyzer::getKnownPatterns() const {
    return knownPatterns;
}

} // namespace midi_transformer