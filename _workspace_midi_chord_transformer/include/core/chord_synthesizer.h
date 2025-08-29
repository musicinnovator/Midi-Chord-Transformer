#pragma once

#include "midi_structures.h"
#include <vector>
#include <string>
#include <memory>

namespace midi_transformer {

// For audio playback of chords
struct ChordAudioPreview {
    std::vector<float> audioSamples;    // PCM audio data
    int sampleRate;                     // e.g., 44100 Hz
    float duration;                     // Duration in seconds
    std::string instrumentName;         // Instrument used for preview
};

// For synthesizer settings
struct SynthSettings {
    std::string waveform;           // "sine", "square", "saw", etc.
    float attack;                   // Attack time in seconds
    float decay;                    // Decay time in seconds
    float sustain;                  // Sustain level (0.0-1.0)
    float release;                  // Release time in seconds
    std::vector<float> harmonics;   // Harmonic amplitudes for custom waveforms
    
    SynthSettings() 
        : waveform("sine"), 
          attack(0.01f), 
          decay(0.1f), 
          sustain(0.7f), 
          release(0.3f) {}
};

class ChordSynthesizer {
private:
    SynthSettings settings;
    int sampleRate;
    
    // Helper methods for synthesis
    std::vector<float> generateWaveform(float frequency, float duration);
    std::vector<float> applyEnvelope(const std::vector<float>& samples);
    std::vector<float> mixSamples(const std::vector<std::vector<float>>& sampleArrays);
    
public:
    ChordSynthesizer(int sr = 44100);
    
    void setSynthSettings(const SynthSettings& newSettings);
    SynthSettings getSynthSettings() const;
    
    std::shared_ptr<ChordAudioPreview> synthesizeChord(const std::vector<uint8_t>& notes, float duration = 2.0f);
    
    bool playChord(const std::vector<uint8_t>& notes, float duration = 2.0f);
    bool playChordComparison(const std::vector<uint8_t>& originalNotes, 
                            const std::vector<uint8_t>& transformedNotes,
                            float duration = 2.0f);
    
    bool saveChordToWav(const std::vector<uint8_t>& notes, const std::string& filename, float duration = 2.0f);
};

} // namespace midi_transformer