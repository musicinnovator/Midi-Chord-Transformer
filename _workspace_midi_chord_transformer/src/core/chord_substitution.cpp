#include "../../include/core/chord_substitution.h"
#include "../../include/utils/midi_utils.h"

#include <algorithm>
#include <unordered_map>
#include <iostream>

namespace midi_transformer {

ChordSubstitutionEngine::ChordSubstitutionEngine() {
    initializeSubstitutionDatabase();
}

void ChordSubstitutionEngine::initializeSubstitutionDatabase() {
    // Initialize common chord substitutions
    
    // Tritone substitutions
    addCustomSubstitution({"G7", "Db7", "tritone sub", 0.3f, 8});
    addCustomSubstitution({"C7", "Gb7", "tritone sub", 0.3f, 8});
    addCustomSubstitution({"F7", "B7", "tritone sub", 0.3f, 8});
    addCustomSubstitution({"Bb7", "E7", "tritone sub", 0.3f, 8});
    addCustomSubstitution({"Eb7", "A7", "tritone sub", 0.3f, 8});
    addCustomSubstitution({"Ab7", "D7", "tritone sub", 0.3f, 8});
    addCustomSubstitution({"Db7", "G7", "tritone sub", 0.3f, 8});
    addCustomSubstitution({"Gb7", "C7", "tritone sub", 0.3f, 8});
    addCustomSubstitution({"B7", "F7", "tritone sub", 0.3f, 8});
    addCustomSubstitution({"E7", "Bb7", "tritone sub", 0.3f, 8});
    addCustomSubstitution({"A7", "Eb7", "tritone sub", 0.3f, 8});
    addCustomSubstitution({"D7", "Ab7", "tritone sub", 0.3f, 8});
    
    // Relative major/minor
    addCustomSubstitution({"C", "Am", "relative minor", -0.2f, 9});
    addCustomSubstitution({"Am", "C", "relative major", 0.2f, 9});
    addCustomSubstitution({"G", "Em", "relative minor", -0.2f, 9});
    addCustomSubstitution({"Em", "G", "relative major", 0.2f, 9});
    addCustomSubstitution({"F", "Dm", "relative minor", -0.2f, 9});
    addCustomSubstitution({"Dm", "F", "relative major", 0.2f, 9});
    
    // Diatonic substitutions
    addCustomSubstitution({"Cmaj7", "Em7", "diatonic sub", -0.1f, 7});
    addCustomSubstitution({"Cmaj7", "Am7", "diatonic sub", -0.1f, 7});
    addCustomSubstitution({"G7", "Bm7b5", "diatonic sub", 0.1f, 6});
    addCustomSubstitution({"Dm7", "Fmaj7", "diatonic sub", 0.1f, 7});
    
    // Modal interchange
    addCustomSubstitution({"C", "Cm", "modal interchange", -0.2f, 8});
    addCustomSubstitution({"Cm", "C", "modal interchange", 0.2f, 8});
    addCustomSubstitution({"F", "Fm", "modal interchange", -0.2f, 8});
    addCustomSubstitution({"Fm", "F", "modal interchange", 0.2f, 8});
    
    // Secondary dominants
    addCustomSubstitution({"Dm7", "A7", "secondary dominant", 0.4f, 5});
    addCustomSubstitution({"G7", "D7", "secondary dominant", 0.4f, 5});
    addCustomSubstitution({"Em7", "B7", "secondary dominant", 0.4f, 5});
    
    // Extended substitutions
    addCustomSubstitution({"C", "C6", "extension", 0.1f, 9});
    addCustomSubstitution({"C", "Cmaj7", "extension", 0.1f, 9});
    addCustomSubstitution({"C", "Cmaj9", "extension", 0.2f, 8});
    addCustomSubstitution({"Cm", "Cm7", "extension", 0.1f, 9});
    addCustomSubstitution({"Cm", "Cm9", "extension", 0.2f, 8});
    addCustomSubstitution({"G7", "G9", "extension", 0.1f, 9});
    addCustomSubstitution({"G7", "G13", "extension", 0.3f, 8});
    
    // Diminished substitutions
    addCustomSubstitution({"G7", "Bdim7", "diminished sub", 0.2f, 7});
    addCustomSubstitution({"C7", "Edim7", "diminished sub", 0.2f, 7});
    
    // Suspended chords
    addCustomSubstitution({"C", "Csus4", "suspended", 0.0f, 8});
    addCustomSubstitution({"G", "Gsus4", "suspended", 0.0f, 8});
    addCustomSubstitution({"G7", "G7sus4", "suspended", 0.0f, 8});
}

SubstitutionOptions ChordSubstitutionEngine::getSubstitutionOptions(const std::string& chordName) {
    SubstitutionOptions options;
    
    // Find all substitutions for this chord
    for (const auto& sub : substitutionDatabase) {
        if (sub.originalChord == chordName) {
            // Categorize by relationship
            if (sub.relationship == "tritone sub" || 
                sub.relationship == "diatonic sub" || 
                sub.relationship == "relative minor" || 
                sub.relationship == "relative major" || 
                sub.relationship == "modal interchange") {
                options.commonSubs.push_back(sub);
            } else if (sub.relationship == "secondary dominant" || 
                       sub.relationship == "diminished sub" || 
                       sub.relationship == "extension") {
                options.jazzSubs.push_back(sub);
            } else if (sub.relationship == "modal interchange") {
                options.modalSubs.push_back(sub);
            } else {
                // Other substitutions go to common subs
                options.commonSubs.push_back(sub);
            }
        }
    }
    
    // Generate reharmonization options
    // This would typically involve more complex chord progressions
    // For simplicity, we'll just add a few examples
    if (chordName == "C") {
        options.reharmonizations.push_back({"C", "Am7 | D7 | Gmaj7", "ii-V-I in G", 0.5f, 6});
        options.reharmonizations.push_back({"C", "Fmaj7 | G7", "IV-V-I", 0.2f, 7});
    } else if (chordName == "G7") {
        options.reharmonizations.push_back({"G7", "Dm7 | G7", "ii-V", 0.3f, 8});
        options.reharmonizations.push_back({"G7", "Db7 | Cmaj7", "tritone sub cadence", 0.4f, 7});
    }
    
    return options;
}

std::vector<ChordSubstitution> ChordSubstitutionEngine::findSubstitutionsByType(
    const std::string& chordName, 
    const std::string& substitutionType) {
    
    std::vector<ChordSubstitution> results;
    
    for (const auto& sub : substitutionDatabase) {
        if (sub.originalChord == chordName && sub.relationship == substitutionType) {
            results.push_back(sub);
        }
    }
    
    return results;
}

std::vector<ChordSubstitution> ChordSubstitutionEngine::findSubstitutionsByFunction(
    const std::string& chordName, 
    int minFunctionalSimilarity) {
    
    std::vector<ChordSubstitution> results;
    
    for (const auto& sub : substitutionDatabase) {
        if (sub.originalChord == chordName && sub.functionalSimilarity >= minFunctionalSimilarity) {
            results.push_back(sub);
        }
    }
    
    // Sort by functional similarity (highest first)
    std::sort(results.begin(), results.end(), 
              [](const ChordSubstitution& a, const ChordSubstitution& b) {
                  return a.functionalSimilarity > b.functionalSimilarity;
              });
    
    return results;
}

std::vector<ChordSubstitution> ChordSubstitutionEngine::findSubstitutionsByTension(
    const std::string& chordName, 
    float minTension, 
    float maxTension) {
    
    std::vector<ChordSubstitution> results;
    
    for (const auto& sub : substitutionDatabase) {
        if (sub.originalChord == chordName && 
            sub.tensionChange >= minTension && 
            sub.tensionChange <= maxTension) {
            results.push_back(sub);
        }
    }
    
    // Sort by tension change (lowest first)
    std::sort(results.begin(), results.end(), 
              [](const ChordSubstitution& a, const ChordSubstitution& b) {
                  return std::abs(a.tensionChange) < std::abs(b.tensionChange);
              });
    
    return results;
}

void ChordSubstitutionEngine::addCustomSubstitution(const ChordSubstitution& substitution) {
    substitutionDatabase.push_back(substitution);
}

} // namespace midi_transformer