#pragma once

#include <string>
#include <vector>
#include <memory>

namespace midi_transformer {

// For suggesting chord substitutions
struct ChordSubstitution {
    std::string originalChord;
    std::string substitutionChord;
    std::string relationship;       // e.g., "tritone sub", "relative minor"
    float tensionChange;            // How much tension changes (-1.0 to 1.0)
    int functionalSimilarity;       // How similar in function (0-10)
};

// For grouping substitution options
struct SubstitutionOptions {
    std::vector<ChordSubstitution> commonSubs;    // Common substitutions
    std::vector<ChordSubstitution> jazzSubs;      // Jazz-oriented substitutions
    std::vector<ChordSubstitution> modalSubs;     // Modal interchange options
    std::vector<ChordSubstitution> reharmonizations; // Complete reharmonization options
};

class ChordSubstitutionEngine {
private:
    std::vector<ChordSubstitution> substitutionDatabase;
    
    void initializeSubstitutionDatabase();
    
public:
    ChordSubstitutionEngine();
    
    SubstitutionOptions getSubstitutionOptions(const std::string& chordName);
    
    std::vector<ChordSubstitution> findSubstitutionsByType(
        const std::string& chordName, 
        const std::string& substitutionType);
    
    std::vector<ChordSubstitution> findSubstitutionsByFunction(
        const std::string& chordName, 
        int minFunctionalSimilarity = 7);
    
    std::vector<ChordSubstitution> findSubstitutionsByTension(
        const std::string& chordName, 
        float minTension = -0.5f, 
        float maxTension = 0.5f);
    
    void addCustomSubstitution(const ChordSubstitution& substitution);
};

} // namespace midi_transformer