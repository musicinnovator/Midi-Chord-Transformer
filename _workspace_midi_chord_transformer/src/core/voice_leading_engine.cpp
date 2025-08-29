#include "../../include/core/voice_leading_engine.h"
#include "../../include/utils/midi_utils.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <iostream>
#include <limits>

namespace midi_transformer {

VoiceLeadingEngine::VoiceLeadingEngine(const VoiceLeadingOptions& opts) {
    options = std::make_shared<VoiceLeadingOptions>(opts);
}

void VoiceLeadingEngine::setOptions(const VoiceLeadingOptions& opts) {
    options = std::make_shared<VoiceLeadingOptions>(opts);
}

VoiceLeadingOptions VoiceLeadingEngine::getOptions() const {
    return *options;
}

std::vector<uint8_t> VoiceLeadingEngine::transformChord(
    const std::vector<uint8_t>& originalNotes,
    const std::string& targetChordName,
    const TransformationOptions& transformOptions) {
    
    // Parse the target chord name to get root and quality
    auto [rootNote, quality] = utils::parseChordName(targetChordName);
    
    // Convert root note to MIDI note number (in octave 4)
    uint8_t rootMidiNote = utils::noteNameToMidi(rootNote);
    
    // Get the target chord notes in a neutral octave
    std::vector<uint8_t> targetChordNotes = utils::getChordNotesFromName(targetChordName, 4);
    
    // Handle different transformation types
    switch (transformOptions.type) {
        case TransformationType::STANDARD: {
            // Standard transformation with voice leading
            if (transformOptions.useVoiceLeading) {
                return findOptimalVoicing(targetChordNotes, originalNotes);
            } else {
                // Just return the target chord notes in the same octave as the original chord
                uint8_t lowestOriginalNote = *std::min_element(originalNotes.begin(), originalNotes.end());
                uint8_t lowestTargetNote = *std::min_element(targetChordNotes.begin(), targetChordNotes.end());
                
                int octaveShift = (lowestOriginalNote / 12) - (lowestTargetNote / 12);
                
                std::vector<uint8_t> result;
                for (uint8_t note : targetChordNotes) {
                    result.push_back(note + (octaveShift * 12));
                }
                
                return result;
            }
        }
        
        case TransformationType::INVERSION: {
            // Apply inversion to the target chord
            std::vector<uint8_t> baseChord = utils::getChordNotesFromName(targetChordName, 4);
            
            // Sort the notes
            std::sort(baseChord.begin(), baseChord.end());
            
            // Apply the inversion
            int inversion = transformOptions.inversion;
            if (inversion < 0) {
                inversion = 0;
            }
            if (inversion >= static_cast<int>(baseChord.size())) {
                inversion = baseChord.size() - 1;
            }
            
            std::vector<uint8_t> invertedChord = baseChord;
            
            // Move notes for inversion
            for (int i = 0; i < inversion; i++) {
                invertedChord[i] += 12;
            }
            
            // Sort again after inversion
            std::sort(invertedChord.begin(), invertedChord.end());
            
            // Apply voice leading to the inverted chord
            if (transformOptions.useVoiceLeading) {
                return findOptimalVoicing(invertedChord, originalNotes);
            } else {
                // Adjust octave to match original chord
                uint8_t lowestOriginalNote = *std::min_element(originalNotes.begin(), originalNotes.end());
                uint8_t lowestInvertedNote = *std::min_element(invertedChord.begin(), invertedChord.end());
                
                int octaveShift = (lowestOriginalNote / 12) - (lowestInvertedNote / 12);
                
                std::vector<uint8_t> result;
                for (uint8_t note : invertedChord) {
                    result.push_back(note + (octaveShift * 12));
                }
                
                return result;
            }
        }
        
        case TransformationType::PERCENTAGE: {
            // Interpolate between original and target chord
            double percentage = transformOptions.percentage;
            if (percentage < 0.0) percentage = 0.0;
            if (percentage > 100.0) percentage = 100.0;
            
            // Get the target chord with optimal voice leading
            std::vector<uint8_t> targetWithVoiceLeading = findOptimalVoicing(targetChordNotes, originalNotes);
            
            // Interpolate between original and target
            std::vector<uint8_t> result;
            
            // Match original notes to target notes
            std::vector<std::pair<uint8_t, uint8_t>> notePairs;
            
            // Create pairs of original and target notes
            if (originalNotes.size() == targetWithVoiceLeading.size()) {
                // Simple case: same number of notes
                for (size_t i = 0; i < originalNotes.size(); i++) {
                    notePairs.push_back({originalNotes[i], targetWithVoiceLeading[i]});
                }
            } else {
                // Complex case: different number of notes
                // For each original note, find the closest target note
                for (uint8_t origNote : originalNotes) {
                    uint8_t closestTargetNote = 0;
                    int minDistance = std::numeric_limits<int>::max();
                    
                    for (uint8_t targetNote : targetWithVoiceLeading) {
                        int distance = std::abs(static_cast<int>(origNote) - static_cast<int>(targetNote));
                        if (distance < minDistance) {
                            minDistance = distance;
                            closestTargetNote = targetNote;
                        }
                    }
                    
                    notePairs.push_back({origNote, closestTargetNote});
                }
                
                // Add any remaining target notes
                for (uint8_t targetNote : targetWithVoiceLeading) {
                    bool found = false;
                    for (const auto& pair : notePairs) {
                        if (pair.second == targetNote) {
                            found = true;
                            break;
                        }
                    }
                    
                    if (!found) {
                        // Find the closest original note
                        uint8_t closestOrigNote = 0;
                        int minDistance = std::numeric_limits<int>::max();
                        
                        for (uint8_t origNote : originalNotes) {
                            int distance = std::abs(static_cast<int>(origNote) - static_cast<int>(targetNote));
                            if (distance < minDistance) {
                                minDistance = distance;
                                closestOrigNote = origNote;
                            }
                        }
                        
                        notePairs.push_back({closestOrigNote, targetNote});
                    }
                }
            }
            
            // Interpolate each note pair
            for (const auto& [origNote, targetNote] : notePairs) {
                int interpolatedNote = static_cast<int>(origNote) + 
                    static_cast<int>((static_cast<double>(targetNote) - static_cast<double>(origNote)) * (percentage / 100.0));
                
                result.push_back(static_cast<uint8_t>(interpolatedNote));
            }
            
            return result;
        }
        
        case TransformationType::SWITCH_TONALITY: {
            // Switch between major and minor tonality
            // This is handled at a higher level in MidiProcessor::switchTonality
            // Here we just apply standard transformation with voice leading
            return findOptimalVoicing(targetChordNotes, originalNotes);
        }
        
        default:
            // Default to standard transformation
            return findOptimalVoicing(targetChordNotes, originalNotes);
    }
}

std::vector<uint8_t> VoiceLeadingEngine::findOptimalVoicing(
    const std::vector<uint8_t>& targetPitches,
    const std::vector<uint8_t>& originalNotes) {
    
    // First, normalize the target pitches to the same octave range
    std::vector<uint8_t> normalizedTargetPitches;
    for (uint8_t pitch : targetPitches) {
        normalizedTargetPitches.push_back(pitch % 12);
    }
    
    // Create all possible voicings of the target chord
    std::vector<std::vector<uint8_t>> possibleVoicings;
    
    // Determine the octave range to consider
    uint8_t minOriginalNote = *std::min_element(originalNotes.begin(), originalNotes.end());
    uint8_t maxOriginalNote = *std::max_element(originalNotes.begin(), originalNotes.end());
    
    int minOctave = std::max(0, static_cast<int>(minOriginalNote / 12) - 1);
    int maxOctave = std::min(10, static_cast<int>(maxOriginalNote / 12) + 1);
    
    // Generate voicings within the octave range
    std::function<void(std::vector<uint8_t>&, size_t, int)> generateVoicings = 
        [&](std::vector<uint8_t>& currentVoicing, size_t index, int currentOctave) {
            if (index == normalizedTargetPitches.size()) {
                possibleVoicings.push_back(currentVoicing);
                return;
            }
            
            for (int octave = minOctave; octave <= maxOctave; octave++) {
                uint8_t pitch = normalizedTargetPitches[index] + (octave * 12);
                
                // Check if the pitch is within MIDI range (0-127)
                if (pitch <= 127) {
                    currentVoicing[index] = pitch;
                    generateVoicings(currentVoicing, index + 1, octave);
                }
            }
        };
    
    std::vector<uint8_t> currentVoicing(normalizedTargetPitches.size());
    generateVoicings(currentVoicing, 0, minOctave);
    
    // Find the voicing with the minimum movement cost
    int minCost = std::numeric_limits<int>::max();
    std::vector<uint8_t> bestVoicing;
    
    for (const auto& voicing : possibleVoicings) {
        // Skip voicings with parallel fifths/octaves if we're avoiding them
        if (options->avoidParallels && hasParallelFifthsOrOctaves(originalNotes, voicing)) {
            continue;
        }
        
        // Calculate movement cost
        int cost = calculateMovementCost(originalNotes, voicing);
        
        if (cost < minCost) {
            minCost = cost;
            bestVoicing = voicing;
        }
    }
    
    // If we couldn't find a valid voicing, just use the first one
    if (bestVoicing.empty() && !possibleVoicings.empty()) {
        bestVoicing = possibleVoicings[0];
    }
    
    // If we still don't have a voicing, use the target pitches in a middle octave
    if (bestVoicing.empty()) {
        for (uint8_t pitch : normalizedTargetPitches) {
            bestVoicing.push_back(pitch + (5 * 12)); // Octave 5
        }
    }
    
    return bestVoicing;
}

bool VoiceLeadingEngine::hasParallelFifthsOrOctaves(
    const std::vector<uint8_t>& originalNotes,
    const std::vector<uint8_t>& newNotes) {
    
    // We need at least 2 notes in each chord to check for parallels
    if (originalNotes.size() < 2 || newNotes.size() < 2) {
        return false;
    }
    
    // Check all pairs of notes for parallel fifths or octaves
    for (size_t i = 0; i < originalNotes.size(); i++) {
        for (size_t j = i + 1; j < originalNotes.size(); j++) {
            // Calculate interval between notes in original chord
            int originalInterval = std::abs(static_cast<int>(originalNotes[i]) - static_cast<int>(originalNotes[j])) % 12;
            
            // Check if this is a fifth (7 semitones) or octave (0 semitones)
            if (originalInterval == 7 || originalInterval == 0) {
                // Find corresponding notes in new chord
                size_t newI = i < newNotes.size() ? i : 0;
                size_t newJ = j < newNotes.size() ? j : newNotes.size() - 1;
                
                // Calculate interval between notes in new chord
                int newInterval = std::abs(static_cast<int>(newNotes[newI]) - static_cast<int>(newNotes[newJ])) % 12;
                
                // Check if the interval is preserved (parallel motion)
                if (newInterval == originalInterval) {
                    // Check if both notes moved in the same direction
                    bool iMoved = originalNotes[i] != newNotes[newI];
                    bool jMoved = originalNotes[j] != newNotes[newJ];
                    
                    if (iMoved && jMoved) {
                        int iDirection = newNotes[newI] > originalNotes[i] ? 1 : -1;
                        int jDirection = newNotes[newJ] > originalNotes[j] ? 1 : -1;
                        
                        if (iDirection == jDirection) {
                            return true; // Parallel fifth or octave found
                        }
                    }
                }
            }
        }
    }
    
    return false;
}

int VoiceLeadingEngine::calculateMovementCost(
    const std::vector<uint8_t>& originalNotes,
    const std::vector<uint8_t>& newNotes) {
    
    int cost = 0;
    
    // If we want to maintain voice count and the counts don't match, apply a high penalty
    if (options->maintainVoiceCount && originalNotes.size() != newNotes.size()) {
        cost += 1000;
    }
    
    // Calculate total movement distance
    std::vector<int> movements;
    
    // Match each original note to the closest new note
    for (uint8_t origNote : originalNotes) {
        int minDistance = std::numeric_limits<int>::max();
        
        for (uint8_t newNote : newNotes) {
            int distance = std::abs(static_cast<int>(newNote) - static_cast<int>(origNote));
            minDistance = std::min(minDistance, distance);
        }
        
        movements.push_back(minDistance);
    }
    
    // Apply cost based on movement distance
    for (int movement : movements) {
        // Penalize movements beyond the max voice movement
        if (movement > options->maxVoiceMovement) {
            cost += (movement - options->maxVoiceMovement) * 10;
        }
        
        // Basic movement cost
        cost += movement;
    }
    
    // If we're minimizing movement, apply a higher weight to the total distance
    if (options->minimizeMovement) {
        cost *= 2;
    }
    
    return cost;
}

std::vector<std::shared_ptr<VoiceMovement>> VoiceLeadingEngine::analyzeVoiceMovement(
    const std::vector<uint8_t>& originalNotes,
    const std::vector<uint8_t>& newNotes) {
    
    std::vector<std::shared_ptr<VoiceMovement>> movements;
    
    // Match each original note to the closest new note
    for (uint8_t origNote : originalNotes) {
        uint8_t closestNewNote = 0;
        int minDistance = std::numeric_limits<int>::max();
        
        for (uint8_t newNote : newNotes) {
            int distance = std::abs(static_cast<int>(newNote) - static_cast<int>(origNote));
            if (distance < minDistance) {
                minDistance = distance;
                closestNewNote = newNote;
            }
        }
        
        auto movement = std::make_shared<VoiceMovement>();
        movement->originalPitch = origNote;
        movement->newPitch = closestNewNote;
        movement->movement = static_cast<int>(closestNewNote) - static_cast<int>(origNote);
        
        // Determine if this is the smallest possible move
        movement->isSmallestPossibleMove = (std::abs(movement->movement) <= options->maxVoiceMovement);
        
        movements.push_back(movement);
    }
    
    // Add any new notes that weren't matched to original notes
    for (uint8_t newNote : newNotes) {
        bool found = false;
        for (const auto& movement : movements) {
            if (movement->newPitch == newNote) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            auto movement = std::make_shared<VoiceMovement>();
            movement->originalPitch = 0; // No original pitch
            movement->newPitch = newNote;
            movement->movement = 0; // New note, not a movement
            movement->isSmallestPossibleMove = true;
            
            movements.push_back(movement);
        }
    }
    
    return movements;
}

} // namespace midi_transformer