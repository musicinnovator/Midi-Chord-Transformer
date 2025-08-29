#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <memory>

namespace midi_transformer {

// MIDI Event Types
enum class MidiEventType {
    NOTE_OFF = 0x80,
    NOTE_ON = 0x90,
    POLY_AFTERTOUCH = 0xA0,
    CONTROL_CHANGE = 0xB0,
    PROGRAM_CHANGE = 0xC0,
    CHANNEL_AFTERTOUCH = 0xD0,
    PITCH_BEND = 0xE0,
    META_EVENT = 0xFF
};

// Meta Event Types
enum class MetaEventType {
    SEQUENCE_NUMBER = 0x00,
    TEXT_EVENT = 0x01,
    COPYRIGHT_NOTICE = 0x02,
    TRACK_NAME = 0x03,
    INSTRUMENT_NAME = 0x04,
    LYRICS = 0x05,
    MARKER = 0x06,
    CUE_POINT = 0x07,
    CHANNEL_PREFIX = 0x20,
    END_OF_TRACK = 0x2F,
    SET_TEMPO = 0x51,
    SMPTE_OFFSET = 0x54,
    TIME_SIGNATURE = 0x58,
    KEY_SIGNATURE = 0x59,
    SEQUENCER_SPECIFIC = 0x7F
};

// MIDI Event Structure
struct MidiEvent {
    uint32_t deltaTime;
    uint8_t status;
    std::vector<uint8_t> data;
    bool isMetaEvent;
    uint8_t metaType;
    
    MidiEvent() : deltaTime(0), status(0), isMetaEvent(false), metaType(0) {}
};

// MIDI Track Structure
struct MidiTrack {
    std::string name;
    std::vector<MidiEvent> events;
    
    MidiTrack() : name("Unnamed Track") {}
};

// MIDI File Structure
struct MidiFile {
    uint16_t format;
    uint16_t numTracks;
    uint16_t division;
    std::vector<MidiTrack> tracks;
    
    MidiFile() : format(1), numTracks(0), division(480) {}
};

// Musical Note Structure
struct Note {
    uint8_t pitch;
    uint32_t startTime;
    uint32_t duration;
    uint8_t velocity;
    uint8_t channel;
    
    Note() : pitch(0), startTime(0), duration(0), velocity(64), channel(0) {}
    
    Note(uint8_t p, uint32_t st, uint32_t d, uint8_t v, uint8_t c)
        : pitch(p), startTime(st), duration(d), velocity(v), channel(c) {}
};

// Musical Chord Structure
struct Chord {
    std::vector<uint8_t> notes;
    std::string name;
    uint32_t startTime;
    uint32_t duration;
    bool isTransformed;
    std::vector<uint8_t> originalNotes;
    std::string originalName;
    
    Chord() : startTime(0), duration(0), isTransformed(false) {}
};

// Transformation Types
enum class TransformationType {
    STANDARD,
    INVERSION,
    PERCENTAGE,
    SWITCH_TONALITY
};

// Transformation Options
struct TransformationOptions {
    TransformationType type;
    int inversion;
    double percentage;
    bool preserveRoot;
    bool preserveBass;
    bool useVoiceLeading;
    
    TransformationOptions() 
        : type(TransformationType::STANDARD), 
          inversion(0), 
          percentage(100.0), 
          preserveRoot(true), 
          preserveBass(true), 
          useVoiceLeading(true) {}
};

} // namespace midi_transformer