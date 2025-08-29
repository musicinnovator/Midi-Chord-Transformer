# MIDI Chord Transformer

A C++ application for detecting and transforming chords in MIDI files with a GUI interface.

## Overview

MIDI Chord Transformer is a powerful tool that allows musicians and composers to:

1. Load and analyze MIDI files to detect chord structures
2. Transform chords using various techniques (standard transformation, inversions, percentage-based morphing, tonality switching)
3. Apply sophisticated voice leading algorithms for smooth chord transitions
4. Analyze chord progressions and detect musical keys
5. Preview chord transformations with audio synthesis
6. Batch process multiple MIDI files

The application features a modern GUI built with ImGui and GLFW, providing an intuitive interface for all operations.

## Features

### Core Functionality
- **MIDI File I/O**: Load and save MIDI files with full event handling
- **Chord Detection**: Sophisticated algorithms to identify chords from note patterns
- **Chord Transformation**: Transform chords while maintaining musical coherence
- **Voice Leading**: Intelligent voice leading to ensure smooth transitions between chords
- **Undo/Redo**: Full history tracking for all transformations

### Advanced Features
- **Chord Progression Analysis**: Identify common chord progressions and patterns
- **Key Detection**: Determine the musical key of a piece
- **Audio Preview**: Listen to original and transformed chords
- **Batch Processing**: Process multiple MIDI files with the same transformations

## Building the Project

### Prerequisites
- CMake 3.10 or higher
- C++17 compatible compiler
- OpenGL
- GLFW library
- ImGui library

### Build Instructions

1. Clone the repository:
   ```
   git clone https://github.com/yourusername/midi-chord-transformer.git
   cd midi-chord-transformer
   ```

2. Create a build directory:
   ```
   mkdir build
   cd build
   ```

3. Configure and build:
   ```
   cmake ..
   make
   ```

4. Run the application:
   ```
   ./midi_chord_transformer
   ```

### External Dependencies

The project requires the following external libraries:
- [ImGui](https://github.com/ocornut/imgui): Immediate mode GUI library
- [GLFW](https://www.glfw.org/): Multi-platform library for OpenGL

These should be placed in the `external` directory before building.

## Usage Guide

### Loading a MIDI File
1. Click "File > Open MIDI File" or press Ctrl+O
2. Select a MIDI file from your system
3. The application will analyze the file and display detected chords

### Transforming Chords
1. Select one or more chords from the chord list
2. Choose a transformation type (Standard, Inversion, Percentage, Switch Tonality)
3. Configure transformation options
4. Click "Transform Selected Chords"
5. Preview the results in the "Transformed Chords" tab

### Saving Transformed MIDI
1. After applying transformations, click "File > Save Transformed MIDI" or press Ctrl+S
2. Choose a location to save the transformed MIDI file

### Batch Processing
1. Click "Tools > Batch Process Directory"
2. Select a directory containing MIDI files
3. Choose which files to process
4. Configure transformation options
5. Click "Process Selected Files"

## Architecture

The application is structured into several key components:

1. **Core Components**:
   - `MidiProcessor`: Central class for MIDI operations and chord transformations
   - `VoiceLeadingEngine`: Handles voice leading algorithms
   - `ChordProgressionAnalyzer`: Analyzes chord progressions
   - `KeyDetector`: Detects musical keys
   - `ChordSynthesizer`: Generates audio previews
   - `ActionManager`: Manages undo/redo functionality

2. **GUI Components**:
   - `MidiChordTransformerApp`: Main application class
   - ImGui-based interface with multiple panels

3. **Utility Components**:
   - MIDI file parsing and writing
   - Music theory utilities
   - File system operations

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- [ImGui](https://github.com/ocornut/imgui) for the GUI framework
- [GLFW](https://www.glfw.org/) for window management and OpenGL context