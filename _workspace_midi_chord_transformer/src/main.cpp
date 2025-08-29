#include "../include/gui/midi_chord_transformer_app.h"
#include <iostream>
#include <exception>

int main(int argc, char** argv) {
    try {
        // Create and run the application
        midi_transformer::MidiChordTransformerApp app;
        app.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }
}