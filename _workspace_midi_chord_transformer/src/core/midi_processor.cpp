#include "../../include/core/midi_processor.h"
#include "../../include/core/chord_progression_analyzer.h"
#include "../../include/core/voice_leading_engine.h"
#include "../../include/core/key_detector.h"
#include "../../include/core/chord_synthesizer.h"
#include "../../include/core/action_manager.h"
#include "../../include/utils/midi_utils.h"

#include <fstream>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <functional>

namespace midi_transformer {

// Constructor
MidiProcessor::MidiProcessor() : timeTolerance(120) {
    // Initialize with default values
    midiFile = std::make_unique<MidiFile>();
    progressionAnalyzer = std::make_unique<ChordProgressionAnalyzer>();
    
    // Initialize voice leading engine with default options
    VoiceLeadingOptions voiceLeadingOpts;
    voiceLeadingEngine = std::make_unique<VoiceLeadingEngine>(voiceLeadingOpts);
    
    keyDetector = std::make_unique<KeyDetector>();
    synthesizer = std::make_unique<ChordSynthesizer>();
    actionManager = std::make_shared<ActionManager>(*this);
}

// MIDI File I/O Methods

bool MidiProcessor::loadMidiFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return false;
    }
    
    // Check if we have a cached analysis for this file
    std::string fileHash = calculateFileHash(filename);
    auto cacheIt = detectionCache.find(fileHash);
    if (cacheIt != detectionCache.end()) {
        // Use cached chord detection
        chords = cacheIt->second->detectedChords;
        currentFilename = filename;
        return true;
    }
    
    // Reset data
    midiFile = std::make_unique<MidiFile>();
    notes.clear();
    chords.clear();
    currentFilename = filename;
    
    // Read file into a buffer
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> buffer(fileSize);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    file.close();
    
    // Parse MIDI header
    if (buffer.size() < 14 || 
        buffer[0] != 'M' || buffer[1] != 'T' || 
        buffer[2] != 'h' || buffer[3] != 'd') {
        std::cerr << "Error: Invalid MIDI file header" << std::endl;
        return false;
    }
    
    size_t position = 4;
    uint32_t headerLength = read32BE(buffer, position);
    position += 4;
    
    midiFile->format = read16BE(buffer, position);
    position += 2;
    
    midiFile->numTracks = read16BE(buffer, position);
    position += 2;
    
    midiFile->division = read16BE(buffer, position);
    position += 2;
    
    // Parse MIDI tracks
    for (uint16_t i = 0; i < midiFile->numTracks; i++) {
        // Check for track header
        if (position + 8 > buffer.size() || 
            buffer[position] != 'M' || buffer[position+1] != 'T' || 
            buffer[position+2] != 'r' || buffer[position+3] != 'k') {
            std::cerr << "Error: Invalid track header at position " << position << std::endl;
            return false;
        }
        
        position += 4;
        uint32_t trackLength = read32BE(buffer, position);
        position += 4;
        
        MidiTrack track;
        size_t trackEnd = position + trackLength;
        
        if (trackEnd > buffer.size()) {
            std::cerr << "Error: Track length exceeds file size" << std::endl;
            return false;
        }
        
        uint8_t runningStatus = 0;
        
        // Parse track events
        while (position < trackEnd) {
            MidiEvent event;
            
            // Read delta time
            event.deltaTime = readVariableLength(buffer, position);
            
            // Read status byte
            if (buffer[position] & 0x80) {
                // This is a status byte
                event.status = buffer[position++];
                runningStatus = event.status;
            } else {
                // Use running status
                event.status = runningStatus;
            }
            
            // Handle different event types
            if (event.status == 0xFF) {
                // Meta event
                event.isMetaEvent = true;
                event.metaType = buffer[position++];
                
                uint32_t length = readVariableLength(buffer, position);
                event.data.assign(buffer.begin() + position, buffer.begin() + position + length);
                position += length;
                
                // Extract track name
                if (event.metaType == static_cast<uint8_t>(MetaEventType::TRACK_NAME)) {
                    track.name = std::string(event.data.begin(), event.data.end());
                }
            } else {
                // MIDI event
                event.isMetaEvent = false;
                
                uint8_t eventType = event.status & 0xF0;
                uint8_t channel = event.status & 0x0F;
                
                switch (eventType) {
                    case static_cast<uint8_t>(MidiEventType::NOTE_OFF):
                    case static_cast<uint8_t>(MidiEventType::NOTE_ON):
                        // Note events have 2 data bytes
                        event.data.push_back(buffer[position++]); // Note number
                        event.data.push_back(buffer[position++]); // Velocity
                        break;
                        
                    case static_cast<uint8_t>(MidiEventType::POLY_AFTERTOUCH):
                        // Polyphonic aftertouch has 2 data bytes
                        event.data.push_back(buffer[position++]); // Note number
                        event.data.push_back(buffer[position++]); // Pressure
                        break;
                        
                    case static_cast<uint8_t>(MidiEventType::CONTROL_CHANGE):
                        // Control change has 2 data bytes
                        event.data.push_back(buffer[position++]); // Controller number
                        event.data.push_back(buffer[position++]); // Value
                        break;
                        
                    case static_cast<uint8_t>(MidiEventType::PROGRAM_CHANGE):
                    case static_cast<uint8_t>(MidiEventType::CHANNEL_AFTERTOUCH):
                        // These events have 1 data byte
                        event.data.push_back(buffer[position++]);
                        break;
                        
                    case static_cast<uint8_t>(MidiEventType::PITCH_BEND):
                        // Pitch bend has 2 data bytes
                        event.data.push_back(buffer[position++]); // LSB
                        event.data.push_back(buffer[position++]); // MSB
                        break;
                        
                    default:
                        // Unknown event type, try to recover
                        std::cerr << "Warning: Unknown event type 0x" 
                                  << std::hex << static_cast<int>(eventType) 
                                  << " at position " << std::dec << position << std::endl;
                        
                        // Skip to the next status byte
                        while (position < trackEnd && !(buffer[position] & 0x80)) {
                            position++;
                        }
                        continue;
                }
            }
            
            track.events.push_back(event);
        }
        
        midiFile->tracks.push_back(track);
    }
    
    // Extract notes and detect chords
    extractNotes();
    detectChords();
    
    // Cache the results
    auto cache = std::make_shared<ChordDetectionCache>();
    cache->midiFileHash = fileHash;
    cache->detectedChords = chords;
    cache->timestamp = std::chrono::system_clock::now();
    detectionCache[fileHash] = cache;
    
    return true;
}

bool MidiProcessor::writeMidiFile(const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for writing" << std::endl;
        return false;
    }
    
    // Create a buffer for the MIDI data
    std::vector<uint8_t> buffer;
    
    // Write MIDI header
    buffer.push_back('M');
    buffer.push_back('T');
    buffer.push_back('h');
    buffer.push_back('d');
    
    // Header length (always 6)
    write32BE(buffer, 6);
    
    // Format
    write16BE(buffer, midiFile->format);
    
    // Number of tracks
    write16BE(buffer, midiFile->numTracks);
    
    // Division (ticks per quarter note)
    write16BE(buffer, midiFile->division);
    
    // Write each track
    for (const auto& track : midiFile->tracks) {
        // Track header
        buffer.push_back('M');
        buffer.push_back('T');
        buffer.push_back('r');
        buffer.push_back('k');
        
        // Placeholder for track length (will be filled in later)
        size_t trackLengthPos = buffer.size();
        write32BE(buffer, 0);
        
        size_t trackStartPos = buffer.size();
        
        // Write track events
        for (const auto& event : track.events) {
            // Write delta time
            writeVariableLength(buffer, event.deltaTime);
            
            // Write status byte
            buffer.push_back(event.status);
            
            if (event.isMetaEvent) {
                // Meta event
                buffer.push_back(event.metaType);
                writeVariableLength(buffer, event.data.size());
                buffer.insert(buffer.end(), event.data.begin(), event.data.end());
            } else {
                // MIDI event
                buffer.insert(buffer.end(), event.data.begin(), event.data.end());
            }
        }
        
        // Calculate and write track length
        uint32_t trackLength = buffer.size() - trackStartPos;
        buffer[trackLengthPos] = (trackLength >> 24) & 0xFF;
        buffer[trackLengthPos + 1] = (trackLength >> 16) & 0xFF;
        buffer[trackLengthPos + 2] = (trackLength >> 8) & 0xFF;
        buffer[trackLengthPos + 3] = trackLength & 0xFF;
    }
    
    // Write the buffer to the file
    file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    file.close();
    
    return true;
}

uint32_t MidiProcessor::readVariableLength(const std::vector<uint8_t>& data, size_t& position) {
    uint32_t value = 0;
    uint8_t byte;
    
    do {
        if (position >= data.size()) {
            std::cerr << "Error: Unexpected end of data while reading variable length value" << std::endl;
            return 0;
        }
        
        byte = data[position++];
        value = (value << 7) | (byte & 0x7F);
    } while (byte & 0x80);
    
    return value;
}

void MidiProcessor::writeVariableLength(std::vector<uint8_t>& data, uint32_t value) {
    std::vector<uint8_t> bytes;
    
    // Extract 7-bit bytes
    bytes.push_back(value & 0x7F);
    while (value >>= 7) {
        bytes.push_back((value & 0x7F) | 0x80);
    }
    
    // Write bytes in reverse order
    for (auto it = bytes.rbegin(); it != bytes.rend(); ++it) {
        data.push_back(*it);
    }
}

uint16_t MidiProcessor::read16BE(const std::vector<uint8_t>& data, size_t position) {
    if (position + 1 >= data.size()) {
        std::cerr << "Error: Unexpected end of data while reading 16-bit value" << std::endl;
        return 0;
    }
    
    return (data[position] << 8) | data[position + 1];
}

void MidiProcessor::write16BE(std::vector<uint8_t>& data, uint16_t value) {
    data.push_back((value >> 8) & 0xFF);
    data.push_back(value & 0xFF);
}

uint32_t MidiProcessor::read32BE(const std::vector<uint8_t>& data, size_t position) {
    if (position + 3 >= data.size()) {
        std::cerr << "Error: Unexpected end of data while reading 32-bit value" << std::endl;
        return 0;
    }
    
    return (data[position] << 24) | (data[position + 1] << 16) | 
           (data[position + 2] << 8) | data[position + 3];
}

void MidiProcessor::write32BE(std::vector<uint8_t>& data, uint32_t value) {
    data.push_back((value >> 24) & 0xFF);
    data.push_back((value >> 16) & 0xFF);
    data.push_back((value >> 8) & 0xFF);
    data.push_back(value & 0xFF);
}

// Chord Detection and Analysis Methods

void MidiProcessor::extractNotes() {
    notes.clear();
    
    // Map to track active notes (key: note number, value: {start time, velocity, channel})
    std::unordered_map<uint8_t, std::tuple<uint32_t, uint8_t, uint8_t>> activeNotes;
    
    // Process each track
    uint32_t absoluteTime = 0;
    
    for (const auto& track : midiFile->tracks) {
        absoluteTime = 0;
        
        for (const auto& event : track.events) {
            absoluteTime += event.deltaTime;
            
            if (!event.isMetaEvent) {
                uint8_t eventType = event.status & 0xF0;
                uint8_t channel = event.status & 0x0F;
                
                if (eventType == static_cast<uint8_t>(MidiEventType::NOTE_ON) && 
                    event.data.size() >= 2) {
                    
                    uint8_t noteNumber = event.data[0];
                    uint8_t velocity = event.data[1];
                    
                    if (velocity > 0) {
                        // Note on event
                        activeNotes[noteNumber] = std::make_tuple(absoluteTime, velocity, channel);
                    } else {
                        // Note on with velocity 0 is equivalent to note off
                        auto it = activeNotes.find(noteNumber);
                        if (it != activeNotes.end()) {
                            uint32_t startTime = std::get<0>(it->second);
                            uint8_t noteVelocity = std::get<1>(it->second);
                            uint8_t noteChannel = std::get<2>(it->second);
                            
                            Note note(noteNumber, startTime, absoluteTime - startTime, 
                                     noteVelocity, noteChannel);
                            notes.push_back(note);
                            
                            activeNotes.erase(it);
                        }
                    }
                } else if (eventType == static_cast<uint8_t>(MidiEventType::NOTE_OFF) && 
                           event.data.size() >= 2) {
                    
                    uint8_t noteNumber = event.data[0];
                    
                    auto it = activeNotes.find(noteNumber);
                    if (it != activeNotes.end()) {
                        uint32_t startTime = std::get<0>(it->second);
                        uint8_t noteVelocity = std::get<1>(it->second);
                        uint8_t noteChannel = std::get<2>(it->second);
                        
                        Note note(noteNumber, startTime, absoluteTime - startTime, 
                                 noteVelocity, noteChannel);
                        notes.push_back(note);
                        
                        activeNotes.erase(it);
                    }
                }
            }
        }
        
        // Handle any notes that are still active at the end of the track
        for (const auto& [noteNumber, noteInfo] : activeNotes) {
            uint32_t startTime = std::get<0>(noteInfo);
            uint8_t noteVelocity = std::get<1>(noteInfo);
            uint8_t noteChannel = std::get<2>(noteInfo);
            
            Note note(noteNumber, startTime, absoluteTime - startTime, 
                     noteVelocity, noteChannel);
            notes.push_back(note);
        }
        
        activeNotes.clear();
    }
    
    // Sort notes by start time
    std::sort(notes.begin(), notes.end(), 
              [](const Note& a, const Note& b) { return a.startTime < b.startTime; });
}

void MidiProcessor::detectChords() {
    chords.clear();
    
    if (notes.empty()) {
        return;
    }
    
    // Group notes by start time (with tolerance)
    std::unordered_map<uint32_t, std::vector<uint8_t>> timeToNotes;
    std::vector<uint32_t> chordStartTimes;
    
    for (const auto& note : notes) {
        // Find a matching chord start time within tolerance
        bool foundMatch = false;
        for (uint32_t startTime : chordStartTimes) {
            if (std::abs(static_cast<int>(note.startTime) - static_cast<int>(startTime)) <= static_cast<int>(timeTolerance)) {
                timeToNotes[startTime].push_back(note.pitch);
                foundMatch = true;
                break;
            }
        }
        
        if (!foundMatch) {
            // Create a new chord start time
            chordStartTimes.push_back(note.startTime);
            timeToNotes[note.startTime].push_back(note.pitch);
        }
    }
    
    // Sort chord start times
    std::sort(chordStartTimes.begin(), chordStartTimes.end());
    
    // Create chords
    for (size_t i = 0; i < chordStartTimes.size(); i++) {
        uint32_t startTime = chordStartTimes[i];
        std::vector<uint8_t> chordNotes = timeToNotes[startTime];
        
        // Remove duplicates
        std::sort(chordNotes.begin(), chordNotes.end());
        chordNotes.erase(std::unique(chordNotes.begin(), chordNotes.end()), chordNotes.end());
        
        // Only consider groups of 3 or more notes as chords
        if (chordNotes.size() >= 3) {
            auto chord = std::make_shared<Chord>();
            chord->notes = chordNotes;
            chord->startTime = startTime;
            
            // Calculate duration (until next chord or end)
            if (i < chordStartTimes.size() - 1) {
                chord->duration = chordStartTimes[i + 1] - startTime;
            } else {
                // Last chord - use the longest note duration
                uint32_t maxDuration = 0;
                for (const auto& note : notes) {
                    if (std::abs(static_cast<int>(note.startTime) - static_cast<int>(startTime)) <= static_cast<int>(timeTolerance)) {
                        maxDuration = std::max(maxDuration, note.duration);
                    }
                }
                chord->duration = maxDuration;
            }
            
            // Identify chord name
            chord->name = identifyChord(chordNotes);
            chord->isTransformed = false;
            
            chords.push_back(chord);
        }
    }
}

std::vector<int> MidiProcessor::normalizeChord(const std::vector<uint8_t>& notes) {
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

std::string MidiProcessor::identifyChord(const std::vector<uint8_t>& notes) {
    if (notes.size() < 3) {
        return "N/A";
    }
    
    // Define common chord types with their interval patterns
    static const std::unordered_map<std::string, std::vector<int>> chordTypes = {
        // Triads
        {"", {0, 4, 7}},           // Major
        {"m", {0, 3, 7}},          // Minor
        {"dim", {0, 3, 6}},        // Diminished
        {"aug", {0, 4, 8}},        // Augmented
        {"sus4", {0, 5, 7}},       // Suspended 4th
        {"sus2", {0, 2, 7}},       // Suspended 2nd
        
        // Seventh chords
        {"7", {0, 4, 7, 10}},      // Dominant 7th
        {"maj7", {0, 4, 7, 11}},   // Major 7th
        {"m7", {0, 3, 7, 10}},     // Minor 7th
        {"dim7", {0, 3, 6, 9}},    // Diminished 7th
        {"m7b5", {0, 3, 6, 10}},   // Half-diminished 7th
        {"aug7", {0, 4, 8, 10}},   // Augmented 7th
        {"7sus4", {0, 5, 7, 10}},  // 7th suspended 4th
        
        // Extended chords
        {"9", {0, 4, 7, 10, 14}},  // Dominant 9th
        {"maj9", {0, 4, 7, 11, 14}}, // Major 9th
        {"m9", {0, 3, 7, 10, 14}}, // Minor 9th
        {"6", {0, 4, 7, 9}},       // Major 6th
        {"m6", {0, 3, 7, 9}},      // Minor 6th
        {"add9", {0, 4, 7, 14}},   // Add 9
        {"madd9", {0, 3, 7, 14}}   // Minor add 9
    };
    
    // Normalize the chord to get intervals
    std::vector<int> intervals = normalizeChord(notes);
    
    // Find the root note
    uint8_t rootNote = *std::min_element(notes.begin(), notes.end());
    
    // Convert root note to name
    static const std::vector<std::string> noteNames = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    
    std::string rootName = noteNames[rootNote % 12];
    
    // Try to match intervals to known chord types
    for (const auto& [quality, pattern] : chordTypes) {
        if (intervals.size() == pattern.size()) {
            bool match = true;
            for (size_t i = 0; i < pattern.size(); i++) {
                if (intervals[i] != pattern[i]) {
                    match = false;
                    break;
                }
            }
            
            if (match) {
                return rootName + quality;
            }
        }
    }
    
    // If no exact match, check for inversions
    for (const auto& [quality, pattern] : chordTypes) {
        if (intervals.size() == pattern.size()) {
            // Try each inversion
            for (size_t inversion = 1; inversion < pattern.size(); inversion++) {
                std::vector<int> invertedPattern = pattern;
                
                // Create the inverted pattern
                for (size_t i = 0; i < inversion; i++) {
                    invertedPattern[i] += 12;
                }
                
                // Sort the inverted pattern
                std::sort(invertedPattern.begin(), invertedPattern.end());
                
                // Compare with our intervals
                bool match = true;
                for (size_t i = 0; i < invertedPattern.size(); i++) {
                    if (intervals[i] != invertedPattern[i]) {
                        match = false;
                        break;
                    }
                }
                
                if (match) {
                    // Find the bass note
                    uint8_t bassNote = notes[0];
                    std::string bassName = noteNames[bassNote % 12];
                    
                    return rootName + quality + "/" + bassName;
                }
            }
        }
    }
    
    // If still no match, return a generic name with the notes
    return rootName + " (" + formatNotes(notes) + ")";
}

std::string MidiProcessor::formatNotes(const std::vector<uint8_t>& notes) {
    static const std::vector<std::string> noteNames = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    
    std::stringstream ss;
    for (size_t i = 0; i < notes.size(); i++) {
        uint8_t note = notes[i];
        ss << noteNames[note % 12] << (note / 12 - 1);
        
        if (i < notes.size() - 1) {
            ss << ", ";
        }
    }
    
    return ss.str();
}

std::pair<std::string, std::string> MidiProcessor::parseChordName(const std::string& chordName) {
    static const std::vector<std::string> noteNames = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    
    // Find the root note
    std::string rootNote;
    size_t pos = 0;
    
    // Check for sharp/flat notes
    if (pos + 1 < chordName.size() && (chordName[pos + 1] == '#' || chordName[pos + 1] == 'b')) {
        rootNote = chordName.substr(pos, 2);
        pos += 2;
    } else {
        rootNote = chordName.substr(pos, 1);
        pos += 1;
    }
    
    // The rest is the chord quality
    std::string quality = chordName.substr(pos);
    
    return {rootNote, quality};
}

// Chord Transformation Methods

std::vector<uint8_t> MidiProcessor::transformChord(
    const std::vector<uint8_t>& notes,
    const std::string& targetChordName,
    const TransformationOptions& options) {
    
    // Use the voice leading engine for transformation
    return voiceLeadingEngine->transformChord(notes, targetChordName, options);
}

void MidiProcessor::transformSelectedChords(
    const std::vector<int>& selectedIndices,
    const std::vector<std::string>& targetChordNames,
    const std::vector<std::shared_ptr<TransformationOptions>>& options) {
    
    // Store original chords for undo history
    std::vector<std::shared_ptr<Chord>> originalChords;
    std::vector<std::shared_ptr<Chord>> transformedChords;
    
    for (size_t i = 0; i < selectedIndices.size(); i++) {
        int index = selectedIndices[i];
        if (index < 0 || index >= static_cast<int>(chords.size())) {
            continue;
        }
        
        // Store original chord
        originalChords.push_back(std::make_shared<Chord>(*chords[index]));
        
        // Transform the chord
        auto& chord = chords[index];
        if (!chord->isTransformed) {
            chord->originalNotes = chord->notes;
            chord->originalName = chord->name;
        }
        
        // Use the VoiceLeadingEngine for transformation
        std::vector<uint8_t> newNotes = transformChord(
            chord->notes, targetChordNames[i], *options[i]);
        
        // Update the chord
        chord->notes = newNotes;
        chord->name = targetChordNames[i];
        chord->isTransformed = true;
        
        // Store transformed chord
        transformedChords.push_back(std::make_shared<Chord>(*chord));
    }
    
    // Record the transformation for undo/redo
    if (!originalChords.empty()) {
        actionManager->recordTransformation(
            selectedIndices, originalChords, transformedChords,
            "Transform " + std::to_string(selectedIndices.size()) + " chords");
    }
}

void MidiProcessor::switchTonality(size_t chordIndex) {
    if (chordIndex >= chords.size()) {
        return;
    }
    
    auto& chord = chords[chordIndex];
    
    // Define tonality switch map
    static const std::unordered_map<std::string, std::string> tonalitySwitch = {
        {"", "m"},           // Major to minor
        {"m", ""},           // Minor to major
        {"dim", "m"},        // Diminished to minor
        {"aug", ""},         // Augmented to major
        {"7", "m7"},         // Dominant 7th to minor 7th
        {"maj7", "m7"},      // Major 7th to minor 7th
        {"m7", "maj7"},      // Minor 7th to major 7th
        {"dim7", "m7b5"},    // Diminished 7th to half-diminished
        {"m7b5", "dim7"},    // Half-diminished to diminished 7th
        {"9", "m9"},         // Dominant 9th to minor 9th
        {"maj9", "m9"},      // Major 9th to minor 9th
        {"m9", "maj9"},      // Minor 9th to major 9th
        {"6", "m6"},         // Major 6th to minor 6th
        {"m6", "6"},         // Minor 6th to major 6th
        {"add9", "madd9"},   // Add 9 to minor add 9
        {"madd9", "add9"}    // Minor add 9 to add 9
    };
    
    // Parse the chord name
    auto [rootNote, quality] = parseChordName(chord->name);
    
    // Check if we have a mapping for this quality
    auto it = tonalitySwitch.find(quality);
    if (it != tonalitySwitch.end()) {
        // Store original chord for undo
        auto originalChord = std::make_shared<Chord>(*chord);
        
        // Create transformation options
        auto options = std::make_shared<TransformationOptions>();
        options->type = TransformationType::SWITCH_TONALITY;
        
        // Create the target chord name
        std::string targetChordName = rootNote + it->second;
        
        // Transform the chord
        if (!chord->isTransformed) {
            chord->originalNotes = chord->notes;
            chord->originalName = chord->name;
        }
        
        std::vector<uint8_t> newNotes = transformChord(chord->notes, targetChordName, *options);
        
        // Update the chord
        chord->notes = newNotes;
        chord->name = targetChordName;
        chord->isTransformed = true;
        
        // Record the transformation for undo/redo
        std::vector<int> indices = {static_cast<int>(chordIndex)};
        std::vector<std::shared_ptr<Chord>> before = {originalChord};
        std::vector<std::shared_ptr<Chord>> after = {std::make_shared<Chord>(*chord)};
        
        actionManager->recordTransformation(
            indices, before, after, "Switch tonality of chord " + std::to_string(chordIndex));
    }
}

// Utility Methods

std::string MidiProcessor::calculateFileHash(const std::string& filename) {
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

void MidiProcessor::setTimeTolerance(uint32_t tolerance) {
    timeTolerance = tolerance;
}

uint32_t MidiProcessor::getTimeTolerance() const {
    return timeTolerance;
}

std::string MidiProcessor::getCurrentFilename() const {
    return currentFilename;
}

std::vector<std::shared_ptr<Chord>> MidiProcessor::getChords() const {
    return chords;
}

std::shared_ptr<Chord> MidiProcessor::getChord(size_t index) const {
    if (index < chords.size()) {
        return chords[index];
    }
    return nullptr;
}

bool MidiProcessor::updateChord(size_t index, const Chord& newChordData) {
    if (index >= chords.size()) return false;
    
    // Create a copy of the current chord for undo history
    auto oldChord = std::make_shared<Chord>(*chords[index]);
    
    // Update the chord with new data
    *chords[index] = newChordData;
    
    return true;
}

void MidiProcessor::displayChords() const {
    std::cout << "Detected Chords:" << std::endl;
    std::cout << "---------------" << std::endl;
    
    for (size_t i = 0; i < chords.size(); i++) {
        const auto& chord = chords[i];
        std::cout << "Chord " << (i + 1) << ": " << chord->name 
                  << " at " << chord->startTime << " ticks, duration: " 
                  << chord->duration << " ticks" << std::endl;
        std::cout << "  Notes: " << formatNotes(chord->notes) << std::endl;
    }
}

void MidiProcessor::displayTransformedChords() const {
    std::cout << "Transformed Chords:" << std::endl;
    std::cout << "------------------" << std::endl;
    
    for (size_t i = 0; i < chords.size(); i++) {
        const auto& chord = chords[i];
        if (chord->isTransformed) {
            std::cout << "Chord " << (i + 1) << ": " << chord->originalName 
                      << " -> " << chord->name << std::endl;
            std::cout << "  Original Notes: " << formatNotes(chord->originalNotes) << std::endl;
            std::cout << "  New Notes: " << formatNotes(chord->notes) << std::endl;
        }
    }
}

bool MidiProcessor::saveChordAnalysis(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for writing" << std::endl;
        return false;
    }
    
    file << "MIDI Chord Analysis" << std::endl;
    file << "===================" << std::endl;
    file << "File: " << currentFilename << std::endl;
    file << "Number of chords: " << chords.size() << std::endl;
    file << std::endl;
    
    file << "Chord List:" << std::endl;
    file << "----------" << std::endl;
    
    for (size_t i = 0; i < chords.size(); i++) {
        const auto& chord = chords[i];
        file << "Chord " << (i + 1) << ": " << chord->name 
             << " at " << chord->startTime << " ticks, duration: " 
             << chord->duration << " ticks" << std::endl;
        file << "  Notes: " << formatNotes(chord->notes) << std::endl;
        
        if (chord->isTransformed) {
            file << "  Original: " << chord->originalName << std::endl;
            file << "  Original Notes: " << formatNotes(chord->originalNotes) << std::endl;
        }
        
        file << std::endl;
    }
    
    file.close();
    return true;
}

void MidiProcessor::analyzeProgression() {
    if (progressionAnalyzer && !chords.empty()) {
        auto progressions = progressionAnalyzer->detectProgressions(chords);
        
        std::cout << "Chord Progression Analysis:" << std::endl;
        std::cout << "--------------------------" << std::endl;
        
        if (progressions.empty()) {
            std::cout << "No recognized progressions found." << std::endl;
        } else {
            for (const auto& prog : progressions) {
                std::cout << "Found progression: " << prog->progressionName 
                          << " (confidence: " << prog->confidence << ")" << std::endl;
                
                std::cout << "  Chords: ";
                for (size_t i = 0; i < prog->chordIndices.size(); i++) {
                    int idx = prog->chordIndices[i];
                    if (idx >= 0 && idx < static_cast<int>(chords.size())) {
                        std::cout << chords[idx]->name;
                        if (i < prog->chordIndices.size() - 1) {
                            std::cout << " -> ";
                        }
                    }
                }
                std::cout << std::endl;
            }
        }
    }
}

void MidiProcessor::detectKey() {
    if (keyDetector && !chords.empty()) {
        auto key = keyDetector->detectKey(chords);
        
        if (key) {
            std::cout << "Key Detection:" << std::endl;
            std::cout << "-------------" << std::endl;
            std::cout << "Detected key: " << key->rootNote 
                      << (key->isMajor ? " Major" : " Minor") << std::endl;
            
            std::cout << "Diatonic chords in this key:" << std::endl;
            for (const auto& [degree, quality] : key->diatonicChords) {
                std::cout << "  " << degree << ": " << quality << std::endl;
            }
        } else {
            std::cout << "Could not determine key with confidence." << std::endl;
        }
    }
}

void MidiProcessor::previewChord(size_t index) {
    if (synthesizer && index < chords.size()) {
        synthesizer->playChord(chords[index]->notes);
    }
}

bool MidiProcessor::undo() {
    return actionManager->undo();
}

bool MidiProcessor::redo() {
    return actionManager->redo();
}

} // namespace midi_transformer