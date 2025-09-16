#pragma once

#include "../core/midi_processor.h"
#include <string>
#include <vector>
#include <memory>

namespace midi_transformer {

class MidiChordTransformerApp {
private:
    std::unique_ptr<MidiProcessor> processor;
    std::vector<std::string> loadedFiles;
    
    // GUI state variables
    std::string inputFilename;
    std::string inputDirectory;
    std::string outputFilename;
    std::string analysisFilename;
    std::vector<bool> selectedChords;
    std::vector<std::string> targetChordNames;
    std::vector<std::shared_ptr<TransformationOptions>> transformOptions;
    std::vector<bool> selectedFiles;
    int currentFileIndex;
    
    // Console output
    std::vector<std::string> consoleOutput;
    
    // Appearance settings
    int currentTheme;
    float uiFontSize;
    float uiRounding;
    bool showAppearanceWindow;
    
    // GUI rendering methods
    void renderMainWindow();
    void renderControlPanel();
    void renderOutputPanel();
    void renderConsoleOutput();
    void renderChordList();
    void renderTransformedChords();
    void renderProgressionAnalysis();
    void renderKeyAnalysis();
    void renderBatchProcessing();
    void renderAppearanceOptions();
    
    // Action handlers
    void handleLoadFile();
    void handleSaveFile();
    void handleBatchProcess();
    void handleTransformChords();
    void handleUndoRedo();
    void handleChordSelection();
    void handleTransformationOptions();
    void handleKeyDetection();
    void handleProgressionAnalysis();
    void handleChordPreview();
    void handleAppearanceChange();
    void applyTheme(int themeIndex);
    
    // Utility methods
    void updateConsoleOutput(const std::string& message);
    void clearConsoleOutput();
    void initializeTransformationOptions();
    void resetChordSelection();
    
public:
    MidiChordTransformerApp();
    
    void run();
    void shutdown();
    
    // Prevent copying but allow moving
    MidiChordTransformerApp(const MidiChordTransformerApp&) = delete;
    MidiChordTransformerApp& operator=(const MidiChordTransformerApp&) = delete;
    MidiChordTransformerApp(MidiChordTransformerApp&&) = default;
    MidiChordTransformerApp& operator=(MidiChordTransformerApp&&) = default;
};

// Console redirection class for capturing output
class ConsoleRedirector {
private:
    std::vector<std::string>& outputBuffer;
    std::streambuf* originalCoutBuffer;
    
public:
    ConsoleRedirector(std::vector<std::string>& buffer);
    ~ConsoleRedirector();
    
    void write(const std::string& message);
};

} // namespace midi_transformer