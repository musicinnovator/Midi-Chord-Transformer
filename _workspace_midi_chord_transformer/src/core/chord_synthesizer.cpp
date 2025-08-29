#include "../../include/core/chord_synthesizer.h"
#include "../../include/utils/midi_utils.h"

#include <cmath>
#include <iostream>
#include <fstream>
#include <algorithm>

namespace midi_transformer {

ChordSynthesizer::ChordSynthesizer(int sr) : sampleRate(sr) {
    // Initialize with default settings
}

void ChordSynthesizer::setSynthSettings(const SynthSettings& newSettings) {
    settings = newSettings;
}

SynthSettings ChordSynthesizer::getSynthSettings() const {
    return settings;
}

std::shared_ptr<ChordAudioPreview> ChordSynthesizer::synthesizeChord(
    const std::vector<uint8_t>& notes, float duration) {
    
    auto preview = std::make_shared<ChordAudioPreview>();
    preview->sampleRate = sampleRate;
    preview->duration = duration;
    preview->instrumentName = settings.waveform;
    
    // Generate waveforms for each note
    std::vector<std::vector<float>> noteWaveforms;
    
    for (uint8_t note : notes) {
        // Convert MIDI note to frequency (A4 = 69 = 440Hz)
        float frequency = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
        
        // Generate the waveform
        std::vector<float> waveform = generateWaveform(frequency, duration);
        
        // Apply envelope
        waveform = applyEnvelope(waveform);
        
        noteWaveforms.push_back(waveform);
    }
    
    // Mix all note waveforms together
    preview->audioSamples = mixSamples(noteWaveforms);
    
    return preview;
}

std::vector<float> ChordSynthesizer::generateWaveform(float frequency, float duration) {
    int numSamples = static_cast<int>(duration * sampleRate);
    std::vector<float> samples(numSamples);
    
    // Generate the waveform based on the selected type
    if (settings.waveform == "sine") {
        // Sine wave
        for (int i = 0; i < numSamples; i++) {
            float t = static_cast<float>(i) / sampleRate;
            samples[i] = std::sin(2.0f * M_PI * frequency * t);
        }
    } else if (settings.waveform == "square") {
        // Square wave
        for (int i = 0; i < numSamples; i++) {
            float t = static_cast<float>(i) / sampleRate;
            float phase = std::fmod(frequency * t, 1.0f);
            samples[i] = phase < 0.5f ? 1.0f : -1.0f;
        }
    } else if (settings.waveform == "saw") {
        // Sawtooth wave
        for (int i = 0; i < numSamples; i++) {
            float t = static_cast<float>(i) / sampleRate;
            float phase = std::fmod(frequency * t, 1.0f);
            samples[i] = 2.0f * phase - 1.0f;
        }
    } else if (settings.waveform == "triangle") {
        // Triangle wave
        for (int i = 0; i < numSamples; i++) {
            float t = static_cast<float>(i) / sampleRate;
            float phase = std::fmod(frequency * t, 1.0f);
            samples[i] = phase < 0.5f ? 
                4.0f * phase - 1.0f : 
                3.0f - 4.0f * phase;
        }
    } else if (settings.waveform == "custom" && !settings.harmonics.empty()) {
        // Custom waveform using harmonics
        for (int i = 0; i < numSamples; i++) {
            float t = static_cast<float>(i) / sampleRate;
            samples[i] = 0.0f;
            
            // Sum the harmonics
            for (size_t h = 0; h < settings.harmonics.size(); h++) {
                float harmFreq = frequency * (h + 1);
                float harmAmp = settings.harmonics[h];
                samples[i] += harmAmp * std::sin(2.0f * M_PI * harmFreq * t);
            }
            
            // Normalize
            samples[i] /= settings.harmonics.size();
        }
    } else {
        // Default to sine wave if unknown type
        for (int i = 0; i < numSamples; i++) {
            float t = static_cast<float>(i) / sampleRate;
            samples[i] = std::sin(2.0f * M_PI * frequency * t);
        }
    }
    
    return samples;
}

std::vector<float> ChordSynthesizer::applyEnvelope(const std::vector<float>& samples) {
    std::vector<float> result = samples;
    
    int numSamples = static_cast<int>(result.size());
    
    // Convert envelope times to sample counts
    int attackSamples = static_cast<int>(settings.attack * sampleRate);
    int decaySamples = static_cast<int>(settings.decay * sampleRate);
    int releaseSamples = static_cast<int>(settings.release * sampleRate);
    
    // Ensure we have enough samples for the envelope
    if (numSamples < attackSamples + decaySamples + releaseSamples) {
        // Scale down envelope times proportionally
        float scale = static_cast<float>(numSamples) / (attackSamples + decaySamples + releaseSamples);
        attackSamples = static_cast<int>(attackSamples * scale);
        decaySamples = static_cast<int>(decaySamples * scale);
        releaseSamples = static_cast<int>(releaseSamples * scale);
    }
    
    // Calculate sustain samples
    int sustainSamples = numSamples - attackSamples - decaySamples - releaseSamples;
    if (sustainSamples < 0) sustainSamples = 0;
    
    // Apply attack phase
    for (int i = 0; i < attackSamples; i++) {
        float envelope = static_cast<float>(i) / attackSamples;
        result[i] *= envelope;
    }
    
    // Apply decay phase
    for (int i = 0; i < decaySamples; i++) {
        float envelope = 1.0f - (1.0f - settings.sustain) * (static_cast<float>(i) / decaySamples);
        result[attackSamples + i] *= envelope;
    }
    
    // Apply sustain phase
    for (int i = 0; i < sustainSamples; i++) {
        result[attackSamples + decaySamples + i] *= settings.sustain;
    }
    
    // Apply release phase
    for (int i = 0; i < releaseSamples; i++) {
        float envelope = settings.sustain * (1.0f - static_cast<float>(i) / releaseSamples);
        result[attackSamples + decaySamples + sustainSamples + i] *= envelope;
    }
    
    return result;
}

std::vector<float> ChordSynthesizer::mixSamples(const std::vector<std::vector<float>>& sampleArrays) {
    if (sampleArrays.empty()) {
        return {};
    }
    
    // Find the maximum length
    size_t maxLength = 0;
    for (const auto& samples : sampleArrays) {
        maxLength = std::max(maxLength, samples.size());
    }
    
    // Mix the samples
    std::vector<float> result(maxLength, 0.0f);
    
    for (const auto& samples : sampleArrays) {
        for (size_t i = 0; i < samples.size(); i++) {
            result[i] += samples[i];
        }
    }
    
    // Normalize to avoid clipping
    float maxAmplitude = 0.0f;
    for (float sample : result) {
        maxAmplitude = std::max(maxAmplitude, std::abs(sample));
    }
    
    if (maxAmplitude > 1.0f) {
        for (float& sample : result) {
            sample /= maxAmplitude;
        }
    }
    
    return result;
}

bool ChordSynthesizer::playChord(const std::vector<uint8_t>& notes, float duration) {
    auto preview = synthesizeChord(notes, duration);
    
    // In a real implementation, this would use an audio API to play the sound
    // For this example, we'll just print a message
    std::cout << "Playing chord: ";
    for (uint8_t note : notes) {
        std::cout << utils::midiNoteToName(note) << " ";
    }
    std::cout << std::endl;
    
    // Return true to indicate success
    return true;
}

bool ChordSynthesizer::playChordComparison(
    const std::vector<uint8_t>& originalNotes, 
    const std::vector<uint8_t>& transformedNotes,
    float duration) {
    
    // In a real implementation, this would play the original chord,
    // pause briefly, then play the transformed chord
    
    std::cout << "Playing chord comparison:" << std::endl;
    
    std::cout << "Original chord: ";
    for (uint8_t note : originalNotes) {
        std::cout << utils::midiNoteToName(note) << " ";
    }
    std::cout << std::endl;
    
    // Play original chord
    auto originalPreview = synthesizeChord(originalNotes, duration);
    
    // Pause
    std::cout << "Pause..." << std::endl;
    
    // Play transformed chord
    std::cout << "Transformed chord: ";
    for (uint8_t note : transformedNotes) {
        std::cout << utils::midiNoteToName(note) << " ";
    }
    std::cout << std::endl;
    
    auto transformedPreview = synthesizeChord(transformedNotes, duration);
    
    // Return true to indicate success
    return true;
}

bool ChordSynthesizer::saveChordToWav(
    const std::vector<uint8_t>& notes, 
    const std::string& filename, 
    float duration) {
    
    auto preview = synthesizeChord(notes, duration);
    
    // In a real implementation, this would write a WAV file
    // For this example, we'll just print a message
    std::cout << "Saving chord to WAV file: " << filename << std::endl;
    
    // Simple WAV file header (44 bytes)
    struct WavHeader {
        // RIFF chunk
        char riffId[4] = {'R', 'I', 'F', 'F'};
        uint32_t riffSize;
        char waveId[4] = {'W', 'A', 'V', 'E'};
        
        // fmt chunk
        char fmtId[4] = {'f', 'm', 't', ' '};
        uint32_t fmtSize = 16;
        uint16_t audioFormat = 1; // PCM
        uint16_t numChannels = 1; // Mono
        uint32_t sampleRate;
        uint32_t byteRate;
        uint16_t blockAlign = 2;
        uint16_t bitsPerSample = 16;
        
        // data chunk
        char dataId[4] = {'d', 'a', 't', 'a'};
        uint32_t dataSize;
    };
    
    // Create WAV header
    WavHeader header;
    header.sampleRate = preview->sampleRate;
    header.byteRate = preview->sampleRate * header.numChannels * (header.bitsPerSample / 8);
    header.dataSize = preview->audioSamples.size() * (header.bitsPerSample / 8);
    header.riffSize = 36 + header.dataSize;
    
    // Open file for writing
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for writing" << std::endl;
        return false;
    }
    
    // Write header
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    
    // Convert float samples to 16-bit PCM
    for (float sample : preview->audioSamples) {
        int16_t pcmSample = static_cast<int16_t>(sample * 32767.0f);
        file.write(reinterpret_cast<const char*>(&pcmSample), sizeof(pcmSample));
    }
    
    file.close();
    
    return true;
}

} // namespace midi_transformer