#include "../../include/core/key_detector.h"
#include "../../include/utils/midi_utils.h"

#include <algorithm>
#include <unordered_map>
#include <iostream>
#include <cmath>

namespace midi_transformer {

KeyDetector::KeyDetector() {
    initializeKeySignatures();
}

void KeyDetector::initializeKeySignatures() {
    // Initialize key signatures for all major and minor keys
    
    // Major keys
    const std::vector<std::string> majorRoots = {
        "C", "G", "D", "A", "E", "B", "F#", "C#", "F", "Bb", "Eb", "Ab", "Db", "Gb", "Cb"
    };
    
    // Minor keys
    const std::vector<std::string> minorRoots = {
        "A", "E", "B", "F#", "C#", "G#", "D#", "A#", "D", "G", "C", "F", "Bb", "Eb", "Ab"
    };
    
    // Create major key signatures
    for (const auto& root : majorRoots) {
        auto key = std::make_shared<KeySignature>();
        key->rootNote = root;
        key->isMajor = true;
        
        // Calculate scale degrees based on root note
        uint8_t rootMidiNote = utils::noteNameToMidi(root) % 12;
        key->scaleDegrees = {
            rootMidiNote,                    // I
            (rootMidiNote + 2) % 12,         // II
            (rootMidiNote + 4) % 12,         // III
            (rootMidiNote + 5) % 12,         // IV
            (rootMidiNote + 7) % 12,         // V
            (rootMidiNote + 9) % 12,         // VI
            (rootMidiNote + 11) % 12         // VII
        };
        
        // Define diatonic chords for major key
        key->diatonicChords = {
            {1, ""},      // I - Major
            {2, "m"},     // ii - Minor
            {3, "m"},     // iii - Minor
            {4, ""},      // IV - Major
            {5, ""},      // V - Major
            {6, "m"},     // vi - Minor
            {7, "dim"}    // vii° - Diminished
        };
        
        keySignatures[root] = key;
    }
    
    // Create minor key signatures
    for (const auto& root : minorRoots) {
        auto key = std::make_shared<KeySignature>();
        key->rootNote = root;
        key->isMajor = false;
        
        // Calculate scale degrees based on root note (natural minor)
        uint8_t rootMidiNote = utils::noteNameToMidi(root) % 12;
        key->scaleDegrees = {
            rootMidiNote,                    // i
            (rootMidiNote + 2) % 12,         // ii
            (rootMidiNote + 3) % 12,         // bIII
            (rootMidiNote + 5) % 12,         // iv
            (rootMidiNote + 7) % 12,         // v
            (rootMidiNote + 8) % 12,         // bVI
            (rootMidiNote + 10) % 12         // bVII
        };
        
        // Define diatonic chords for minor key
        key->diatonicChords = {
            {1, "m"},     // i - Minor
            {2, "dim"},   // ii° - Diminished
            {3, ""},      // bIII - Major
            {4, "m"},     // iv - Minor
            {5, "m"},     // v - Minor (or "7" for harmonic minor)
            {6, ""},      // bVI - Major
            {7, ""}       // bVII - Major
        };
        
        // Store with "m" suffix to distinguish from major keys
        keySignatures[root + "m"] = key;
    }
}

std::shared_ptr<KeySignature> KeyDetector::detectKey(const std::vector<std::shared_ptr<Chord>>& chords) {
    if (chords.empty()) {
        return nullptr;
    }
    
    // Count occurrences of each pitch class
    std::vector<int> pitchClassCounts(12, 0);
    
    // Extract all notes from all chords
    for (const auto& chord : chords) {
        for (uint8_t note : chord->notes) {
            pitchClassCounts[note % 12]++;
        }
    }
    
    // Calculate key scores for each possible key
    std::unordered_map<std::string, double> keyScores;
    
    for (const auto& [keyName, key] : keySignatures) {
        // Count how many notes are in the key
        int notesInKey = 0;
        int totalNotes = 0;
        
        for (int i = 0; i < 12; i++) {
            if (pitchClassCounts[i] > 0) {
                totalNotes += pitchClassCounts[i];
                
                // Check if this pitch class is in the key
                if (std::find(key->scaleDegrees.begin(), key->scaleDegrees.end(), i) != key->scaleDegrees.end()) {
                    notesInKey += pitchClassCounts[i];
                }
            }
        }
        
        // Calculate score as percentage of notes in key
        double score = totalNotes > 0 ? static_cast<double>(notesInKey) / totalNotes : 0.0;
        
        // Weight certain scale degrees more heavily
        // Tonic, dominant, and subdominant are more important
        int tonic = utils::noteNameToMidi(key->rootNote) % 12;
        int dominant = (tonic + 7) % 12;
        int subdominant = (tonic + 5) % 12;
        
        if (pitchClassCounts[tonic] > 0) {
            score *= 1.2; // Boost for tonic presence
        }
        if (pitchClassCounts[dominant] > 0) {
            score *= 1.1; // Boost for dominant presence
        }
        if (pitchClassCounts[subdominant] > 0) {
            score *= 1.05; // Boost for subdominant presence
        }
        
        // Check for chord progressions that strongly indicate a key
        // This is a simplified approach - a real implementation would be more sophisticated
        bool hasTonicChord = false;
        bool hasDominantChord = false;
        bool hasSubdominantChord = false;
        
        for (const auto& chord : chords) {
            std::string root = utils::getChordRoot(chord->name);
            std::string quality = utils::getChordQuality(chord->name);
            
            uint8_t rootNote = utils::noteNameToMidi(root) % 12;
            
            if (rootNote == tonic) {
                if ((key->isMajor && (quality.empty() || quality == "maj7" || quality == "6")) ||
                    (!key->isMajor && (quality == "m" || quality == "m7"))) {
                    hasTonicChord = true;
                }
            } else if (rootNote == dominant) {
                if ((quality.empty() || quality == "7")) {
                    hasDominantChord = true;
                }
            } else if (rootNote == subdominant) {
                if ((key->isMajor && (quality.empty() || quality == "maj7")) ||
                    (!key->isMajor && (quality == "m" || quality == "m7"))) {
                    hasSubdominantChord = true;
                }
            }
        }
        
        if (hasTonicChord) score *= 1.3;
        if (hasDominantChord) score *= 1.2;
        if (hasSubdominantChord) score *= 1.1;
        
        // Store the score
        keyScores[keyName] = score;
    }
    
    // Find the key with the highest score
    std::string bestKey;
    double bestScore = -1.0;
    
    for (const auto& [keyName, score] : keyScores) {
        if (score > bestScore) {
            bestScore = score;
            bestKey = keyName;
        }
    }
    
    // Return the best key if score is above threshold
    if (bestScore >= 0.6) {
        return keySignatures[bestKey];
    }
    
    // If no clear key is found, return nullptr
    return nullptr;
}

std::vector<std::shared_ptr<ScaleConstraint>> KeyDetector::getScaleConstraints(
    const std::shared_ptr<KeySignature>& key) {
    
    std::vector<std::shared_ptr<ScaleConstraint>> constraints;
    
    if (!key) {
        return constraints;
    }
    
    // Create constraint for the main scale
    auto mainScale = std::make_shared<ScaleConstraint>();
    mainScale->scaleType = key->isMajor ? "major" : "minor";
    mainScale->rootNote = utils::noteNameToMidi(key->rootNote) % 12;
    mainScale->allowedNotes = key->scaleDegrees;
    
    // Add allowed chords based on diatonic chords
    for (const auto& [degree, quality] : key->diatonicChords) {
        // Calculate root note of this chord
        int degreeIndex = degree - 1; // Convert from 1-based to 0-based
        if (degreeIndex >= 0 && degreeIndex < static_cast<int>(key->scaleDegrees.size())) {
            uint8_t chordRoot = key->scaleDegrees[degreeIndex];
            std::string rootName = utils::midiNoteToName(chordRoot);
            mainScale->allowedChords.push_back(rootName + quality);
        }
    }
    
    constraints.push_back(mainScale);
    
    // Add additional constraints for common modal interchange scales
    if (key->isMajor) {
        // Add parallel minor (for modal interchange)
        auto parallelMinor = std::make_shared<ScaleConstraint>();
        parallelMinor->scaleType = "parallel minor";
        parallelMinor->rootNote = mainScale->rootNote;
        
        // Calculate natural minor scale degrees
        uint8_t root = parallelMinor->rootNote;
        parallelMinor->allowedNotes = {
            root,
            (root + 2) % 12,
            (root + 3) % 12,
            (root + 5) % 12,
            (root + 7) % 12,
            (root + 8) % 12,
            (root + 10) % 12
        };
        
        // Add common borrowed chords
        std::string rootName = utils::midiNoteToName(root);
        parallelMinor->allowedChords = {
            rootName + "m",                                  // i
            utils::midiNoteToName((root + 3) % 12),          // bIII
            utils::midiNoteToName((root + 5) % 12) + "m",    // iv
            utils::midiNoteToName((root + 8) % 12),          // bVI
            utils::midiNoteToName((root + 10) % 12)          // bVII
        };
        
        constraints.push_back(parallelMinor);
    } else {
        // Add harmonic minor
        auto harmonicMinor = std::make_shared<ScaleConstraint>();
        harmonicMinor->scaleType = "harmonic minor";
        harmonicMinor->rootNote = mainScale->rootNote;
        
        // Calculate harmonic minor scale degrees
        uint8_t root = harmonicMinor->rootNote;
        harmonicMinor->allowedNotes = {
            root,
            (root + 2) % 12,
            (root + 3) % 12,
            (root + 5) % 12,
            (root + 7) % 12,
            (root + 8) % 12,
            (root + 11) % 12 // Raised 7th
        };
        
        // Add common harmonic minor chords
        std::string rootName = utils::midiNoteToName(root);
        harmonicMinor->allowedChords = {
            rootName + "m",                                  // i
            utils::midiNoteToName((root + 7) % 12) + "7",    // V7 (dominant)
            utils::midiNoteToName((root + 11) % 12) + "dim7" // vii°7
        };
        
        constraints.push_back(harmonicMinor);
        
        // Add melodic minor
        auto melodicMinor = std::make_shared<ScaleConstraint>();
        melodicMinor->scaleType = "melodic minor";
        melodicMinor->rootNote = mainScale->rootNote;
        
        // Calculate melodic minor scale degrees (ascending)
        root = melodicMinor->rootNote;
        melodicMinor->allowedNotes = {
            root,
            (root + 2) % 12,
            (root + 3) % 12,
            (root + 5) % 12,
            (root + 7) % 12,
            (root + 9) % 12, // Raised 6th
            (root + 11) % 12 // Raised 7th
        };
        
        // Add common melodic minor chords
        rootName = utils::midiNoteToName(root);
        melodicMinor->allowedChords = {
            rootName + "m6",                                 // i6
            utils::midiNoteToName((root + 7) % 12) + "7",    // V7
            utils::midiNoteToName((root + 9) % 12) + "m7b5"  // vi half-diminished
        };
        
        constraints.push_back(melodicMinor);
    }
    
    return constraints;
}

std::shared_ptr<KeySignature> KeyDetector::getKeySignature(const std::string& keyName) {
    auto it = keySignatures.find(keyName);
    if (it != keySignatures.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::string> KeyDetector::getAllKeyNames() const {
    std::vector<std::string> names;
    names.reserve(keySignatures.size());
    
    for (const auto& [name, _] : keySignatures) {
        names.push_back(name);
    }
    
    return names;
}

int KeyDetector::countNotesInKey(const std::vector<uint8_t>& notes, const std::shared_ptr<KeySignature>& key) {
    int count = 0;
    
    for (uint8_t note : notes) {
        uint8_t pitchClass = note % 12;
        if (std::find(key->scaleDegrees.begin(), key->scaleDegrees.end(), pitchClass) != key->scaleDegrees.end()) {
            count++;
        }
    }
    
    return count;
}

} // namespace midi_transformer