#include "../../include/gui/midi_chord_transformer_app.h"
#include "../../include/utils/midi_utils.h"

#include <iostream>
#include <algorithm>
#include <filesystem>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

namespace midi_transformer {

// GLFW error callback
static void glfw_error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

MidiChordTransformerApp::MidiChordTransformerApp() {
    processor = std::make_unique<MidiProcessor>();
    inputFilename.resize(256);
    inputDirectory.resize(256);
    outputFilename.resize(256);
    analysisFilename.resize(256);
    currentFileIndex = 0;
    
    initializeTransformationOptions();
}

void MidiChordTransformerApp::run() {
    // Setup GLFW window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return;
    }
    
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    
    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "MIDI Chord Transformer", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Poll and handle events
        glfwPollEvents();
        
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Render the main window
        renderMainWindow();
        
        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(window);
    }
    
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    glfwDestroyWindow(window);
    glfwTerminate();
}

void MidiChordTransformerApp::shutdown() {
    // Additional cleanup if needed
}

void MidiChordTransformerApp::renderMainWindow() {
    // Create a full-window frame
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("MIDI Chord Transformer", nullptr, 
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar);
    
    // Menu bar
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open MIDI File", "Ctrl+O")) {
                handleLoadFile();
            }
            if (ImGui::MenuItem("Save Transformed MIDI", "Ctrl+S")) {
                handleSaveFile();
            }
            if (ImGui::MenuItem("Save Chord Analysis", "Ctrl+A")) {
                if (!processor->getCurrentFilename().empty()) {
                    std::string baseFilename = utils::getBaseFilename(processor->getCurrentFilename());
                    std::string analysisFile = baseFilename + "_analysis_" + utils::generateTimestamp() + ".txt";
                    if (processor->saveChordAnalysis(analysisFile)) {
                        updateConsoleOutput("Chord analysis saved to " + analysisFile);
                    } else {
                        updateConsoleOutput("Failed to save chord analysis");
                    }
                } else {
                    updateConsoleOutput("No MIDI file loaded");
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                glfwSetWindowShouldClose(ImGui::GetCurrentContext()->PlatformHandleRaw, true);
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
                if (processor->undo()) {
                    updateConsoleOutput("Undo successful");
                } else {
                    updateConsoleOutput("Nothing to undo");
                }
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
                if (processor->redo()) {
                    updateConsoleOutput("Redo successful");
                } else {
                    updateConsoleOutput("Nothing to redo");
                }
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Tools")) {
            if (ImGui::MenuItem("Detect Key")) {
                processor->detectKey();
            }
            if (ImGui::MenuItem("Analyze Progression")) {
                processor->analyzeProgression();
            }
            if (ImGui::MenuItem("Batch Process Directory")) {
                handleBatchProcess();
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                updateConsoleOutput("MIDI Chord Transformer v1.0");
                updateConsoleOutput("A tool for detecting and transforming chords in MIDI files");
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }
    
    // Split the window into two panels
    ImGui::Columns(2, "MainSplit", true);
    
    // Left panel - Control panel
    renderControlPanel();
    
    // Right panel - Output panel
    ImGui::NextColumn();
    renderOutputPanel();
    
    ImGui::Columns(1);
    ImGui::End();
}

void MidiChordTransformerApp::renderControlPanel() {
    ImGui::BeginChild("ControlPanel", ImVec2(0, 0), true);
    ImGui::Text("Control Panel");
    ImGui::Separator();
    
    // File selection
    ImGui::Text("MIDI File:");
    ImGui::InputText("##InputFile", &inputFilename[0], inputFilename.size());
    ImGui::SameLine();
    if (ImGui::Button("Browse...")) {
        handleLoadFile();
    }
    
    // Display current file info
    if (!processor->getCurrentFilename().empty()) {
        ImGui::Text("Current File: %s", processor->getCurrentFilename().c_str());
        
        auto chords = processor->getChords();
        ImGui::Text("Detected Chords: %zu", chords.size());
        
        // Resize selection vectors if needed
        if (selectedChords.size() != chords.size()) {
            selectedChords.resize(chords.size(), false);
            targetChordNames.resize(chords.size());
            transformOptions.resize(chords.size());
            
            // Initialize transformation options
            for (size_t i = 0; i < chords.size(); i++) {
                if (!transformOptions[i]) {
                    transformOptions[i] = std::make_shared<TransformationOptions>();
                }
                targetChordNames[i] = chords[i]->name;
            }
        }
        
        // Chord selection
        ImGui::Separator();
        ImGui::Text("Chord Selection:");
        
        if (ImGui::Button("Select All")) {
            std::fill(selectedChords.begin(), selectedChords.end(), true);
        }
        ImGui::SameLine();
        if (ImGui::Button("Deselect All")) {
            std::fill(selectedChords.begin(), selectedChords.end(), false);
        }
        
        // Transformation options
        ImGui::Separator();
        ImGui::Text("Transformation Options:");
        
        // Transformation type
        static int transformType = 0;
        ImGui::Combo("Transformation Type", &transformType, "Standard\0Inversion\0Percentage\0Switch Tonality\0");
        
        // Options based on transformation type
        switch (transformType) {
            case 0: // Standard
                break;
                
            case 1: // Inversion
                static int inversion = 0;
                ImGui::SliderInt("Inversion", &inversion, 0, 3);
                break;
                
            case 2: // Percentage
                static float percentage = 100.0f;
                ImGui::SliderFloat("Percentage", &percentage, 0.0f, 100.0f, "%.1f%%");
                break;
                
            case 3: // Switch Tonality
                break;
        }
        
        // Voice leading options
        static bool useVoiceLeading = true;
        ImGui::Checkbox("Use Voice Leading", &useVoiceLeading);
        
        static bool preserveRoot = true;
        ImGui::Checkbox("Preserve Root", &preserveRoot);
        
        static bool preserveBass = true;
        ImGui::Checkbox("Preserve Bass", &preserveBass);
        
        // Apply transformation button
        ImGui::Separator();
        if (ImGui::Button("Transform Selected Chords")) {
            handleTransformChords();
        }
        
        // Update transformation options for all selected chords
        for (size_t i = 0; i < transformOptions.size(); i++) {
            if (selectedChords[i]) {
                transformOptions[i]->type = static_cast<TransformationType>(transformType);
                
                switch (transformType) {
                    case 1: // Inversion
                        transformOptions[i]->inversion = inversion;
                        break;
                        
                    case 2: // Percentage
                        transformOptions[i]->percentage = percentage;
                        break;
                }
                
                transformOptions[i]->useVoiceLeading = useVoiceLeading;
                transformOptions[i]->preserveRoot = preserveRoot;
                transformOptions[i]->preserveBass = preserveBass;
            }
        }
    }
    
    ImGui::EndChild();
}

void MidiChordTransformerApp::renderOutputPanel() {
    ImGui::BeginChild("OutputPanel", ImVec2(0, 0), true);
    
    // Tabs for different output views
    if (ImGui::BeginTabBar("OutputTabs")) {
        if (ImGui::BeginTabItem("Console Output")) {
            renderConsoleOutput();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Chord List")) {
            renderChordList();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Transformed Chords")) {
            renderTransformedChords();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Progression Analysis")) {
            renderProgressionAnalysis();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Key Analysis")) {
            renderKeyAnalysis();
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    
    ImGui::EndChild();
}

void MidiChordTransformerApp::renderConsoleOutput() {
    ImGui::BeginChild("ConsoleOutput", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true);
    
    for (const auto& line : consoleOutput) {
        ImGui::TextWrapped("%s", line.c_str());
    }
    
    // Auto-scroll to bottom
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    
    ImGui::EndChild();
    
    // Clear button
    if (ImGui::Button("Clear Console")) {
        clearConsoleOutput();
    }
}

void MidiChordTransformerApp::renderChordList() {
    auto chords = processor->getChords();
    
    if (chords.empty()) {
        ImGui::Text("No chords detected. Load a MIDI file first.");
        return;
    }
    
    ImGui::BeginChild("ChordList", ImVec2(0, 0), true);
    
    // Table header
    ImGui::Columns(5, "ChordListColumns", true);
    ImGui::Text("Select"); ImGui::NextColumn();
    ImGui::Text("Chord #"); ImGui::NextColumn();
    ImGui::Text("Name"); ImGui::NextColumn();
    ImGui::Text("Time"); ImGui::NextColumn();
    ImGui::Text("Notes"); ImGui::NextColumn();
    ImGui::Separator();
    
    // Table rows
    for (size_t i = 0; i < chords.size(); i++) {
        const auto& chord = chords[i];
        
        // Checkbox for selection
        ImGui::PushID(static_cast<int>(i));
        ImGui::Checkbox("##select", &selectedChords[i]);
        ImGui::NextColumn();
        
        // Chord number
        ImGui::Text("%zu", i + 1);
        ImGui::NextColumn();
        
        // Chord name
        ImGui::Text("%s", chord->name.c_str());
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Click to edit chord name");
            ImGui::EndTooltip();
        }
        if (ImGui::IsItemClicked()) {
            // TODO: Implement chord name editing
        }
        ImGui::NextColumn();
        
        // Chord time
        ImGui::Text("%u", chord->startTime);
        ImGui::NextColumn();
        
        // Chord notes
        std::string noteStr = utils::formatChordNotes(chord->notes);
        ImGui::Text("%s", noteStr.c_str());
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Click to preview chord");
            ImGui::EndTooltip();
        }
        if (ImGui::IsItemClicked()) {
            processor->previewChord(i);
        }
        ImGui::NextColumn();
        
        ImGui::PopID();
    }
    
    ImGui::Columns(1);
    ImGui::EndChild();
}

void MidiChordTransformerApp::renderTransformedChords() {
    auto chords = processor->getChords();
    
    if (chords.empty()) {
        ImGui::Text("No chords detected. Load a MIDI file first.");
        return;
    }
    
    // Count transformed chords
    int transformedCount = 0;
    for (const auto& chord : chords) {
        if (chord->isTransformed) {
            transformedCount++;
        }
    }
    
    if (transformedCount == 0) {
        ImGui::Text("No chords have been transformed yet.");
        return;
    }
    
    ImGui::BeginChild("TransformedChords", ImVec2(0, 0), true);
    
    // Table header
    ImGui::Columns(5, "TransformedColumns", true);
    ImGui::Text("Chord #"); ImGui::NextColumn();
    ImGui::Text("Original"); ImGui::NextColumn();
    ImGui::Text("Transformed"); ImGui::NextColumn();
    ImGui::Text("Original Notes"); ImGui::NextColumn();
    ImGui::Text("New Notes"); ImGui::NextColumn();
    ImGui::Separator();
    
    // Table rows
    for (size_t i = 0; i < chords.size(); i++) {
        const auto& chord = chords[i];
        
        if (chord->isTransformed) {
            ImGui::PushID(static_cast<int>(i));
            
            // Chord number
            ImGui::Text("%zu", i + 1);
            ImGui::NextColumn();
            
            // Original chord name
            ImGui::Text("%s", chord->originalName.c_str());
            ImGui::NextColumn();
            
            // Transformed chord name
            ImGui::Text("%s", chord->name.c_str());
            ImGui::NextColumn();
            
            // Original notes
            std::string origNoteStr = utils::formatChordNotes(chord->originalNotes);
            ImGui::Text("%s", origNoteStr.c_str());
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Click to preview original chord");
                ImGui::EndTooltip();
            }
            // TODO: Implement preview of original chord
            ImGui::NextColumn();
            
            // New notes
            std::string newNoteStr = utils::formatChordNotes(chord->notes);
            ImGui::Text("%s", newNoteStr.c_str());
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Click to preview transformed chord");
                ImGui::EndTooltip();
            }
            if (ImGui::IsItemClicked()) {
                processor->previewChord(i);
            }
            ImGui::NextColumn();
            
            ImGui::PopID();
        }
    }
    
    ImGui::Columns(1);
    ImGui::EndChild();
}

void MidiChordTransformerApp::renderProgressionAnalysis() {
    ImGui::Text("Progression Analysis");
    ImGui::Separator();
    
    if (processor->getChords().empty()) {
        ImGui::Text("No chords detected. Load a MIDI file first.");
        return;
    }
    
    if (ImGui::Button("Analyze Progression")) {
        processor->analyzeProgression();
    }
    
    // In a real implementation, this would display the results of the progression analysis
    ImGui::Text("This panel would show chord progression analysis results.");
}

void MidiChordTransformerApp::renderKeyAnalysis() {
    ImGui::Text("Key Analysis");
    ImGui::Separator();
    
    if (processor->getChords().empty()) {
        ImGui::Text("No chords detected. Load a MIDI file first.");
        return;
    }
    
    if (ImGui::Button("Detect Key")) {
        processor->detectKey();
    }
    
    // In a real implementation, this would display the results of the key analysis
    ImGui::Text("This panel would show key detection results.");
}

void MidiChordTransformerApp::renderBatchProcessing() {
    ImGui::Text("Batch Processing");
    ImGui::Separator();
    
    ImGui::Text("Directory:");
    ImGui::InputText("##InputDir", &inputDirectory[0], inputDirectory.size());
    ImGui::SameLine();
    if (ImGui::Button("Browse...")) {
        // TODO: Implement directory browser
    }
    
    if (ImGui::Button("Find MIDI Files")) {
        loadedFiles = utils::findMidiFiles(inputDirectory);
        selectedFiles.resize(loadedFiles.size(), false);
        updateConsoleOutput("Found " + std::to_string(loadedFiles.size()) + " MIDI files");
    }
    
    // Display found files
    if (!loadedFiles.empty()) {
        ImGui::Text("Found %zu MIDI files:", loadedFiles.size());
        
        ImGui::BeginChild("FileList", ImVec2(0, 200), true);
        for (size_t i = 0; i < loadedFiles.size(); i++) {
            ImGui::Checkbox(loadedFiles[i].c_str(), &selectedFiles[i]);
        }
        ImGui::EndChild();
        
        if (ImGui::Button("Process Selected Files")) {
            handleBatchProcess();
        }
    }
}

void MidiChordTransformerApp::handleLoadFile() {
    // In a real implementation, this would open a file dialog
    // For this example, we'll use a hardcoded filename
    std::string filename = "example.mid";
    
    if (processor->loadMidiFile(filename)) {
        updateConsoleOutput("Loaded MIDI file: " + filename);
        
        // Reset selection and options
        resetChordSelection();
    } else {
        updateConsoleOutput("Failed to load MIDI file: " + filename);
    }
}

void MidiChordTransformerApp::handleSaveFile() {
    if (processor->getCurrentFilename().empty()) {
        updateConsoleOutput("No MIDI file loaded");
        return;
    }
    
    // Generate output filename
    std::string baseFilename = utils::getBaseFilename(processor->getCurrentFilename());
    std::string outputFile = baseFilename + "_transformed_" + utils::generateTimestamp() + ".mid";
    
    if (processor->writeMidiFile(outputFile)) {
        updateConsoleOutput("Saved transformed MIDI to " + outputFile);
    } else {
        updateConsoleOutput("Failed to save transformed MIDI");
    }
}

void MidiChordTransformerApp::handleBatchProcess() {
    // Count selected files
    int selectedCount = 0;
    for (bool selected : selectedFiles) {
        if (selected) {
            selectedCount++;
        }
    }
    
    if (selectedCount == 0) {
        updateConsoleOutput("No files selected for batch processing");
        return;
    }
    
    updateConsoleOutput("Starting batch processing of " + std::to_string(selectedCount) + " files");
    
    // Process each selected file
    int processedCount = 0;
    for (size_t i = 0; i < loadedFiles.size(); i++) {
        if (selectedFiles[i]) {
            updateConsoleOutput("Processing " + loadedFiles[i]);
            
            if (processor->loadMidiFile(loadedFiles[i])) {
                // Apply transformations
                // TODO: Implement batch transformation logic
                
                // Save transformed file
                std::string baseFilename = utils::getBaseFilename(loadedFiles[i]);
                std::string outputFile = baseFilename + "_transformed_" + utils::generateTimestamp() + ".mid";
                
                if (processor->writeMidiFile(outputFile)) {
                    updateConsoleOutput("Saved transformed MIDI to " + outputFile);
                    processedCount++;
                } else {
                    updateConsoleOutput("Failed to save transformed MIDI for " + loadedFiles[i]);
                }
            } else {
                updateConsoleOutput("Failed to load MIDI file: " + loadedFiles[i]);
            }
        }
    }
    
    updateConsoleOutput("Batch processing complete. Processed " + std::to_string(processedCount) + 
                       " out of " + std::to_string(selectedCount) + " files");
}

void MidiChordTransformerApp::handleTransformChords() {
    auto chords = processor->getChords();
    
    if (chords.empty()) {
        updateConsoleOutput("No chords to transform");
        return;
    }
    
    // Count selected chords
    int selectedCount = 0;
    for (bool selected : selectedChords) {
        if (selected) {
            selectedCount++;
        }
    }
    
    if (selectedCount == 0) {
        updateConsoleOutput("No chords selected for transformation");
        return;
    }
    
    // Collect selected chord indices and their target chord names
    std::vector<int> selectedIndices;
    std::vector<std::string> targetNames;
    std::vector<std::shared_ptr<TransformationOptions>> options;
    
    for (size_t i = 0; i < selectedChords.size(); i++) {
        if (selectedChords[i]) {
            selectedIndices.push_back(static_cast<int>(i));
            targetNames.push_back(targetChordNames[i]);
            options.push_back(transformOptions[i]);
        }
    }
    
    // Apply transformations
    processor->transformSelectedChords(selectedIndices, targetNames, options);
    
    updateConsoleOutput("Transformed " + std::to_string(selectedCount) + " chords");
}

void MidiChordTransformerApp::handleUndoRedo() {
    // Implemented in menu callbacks
}

void MidiChordTransformerApp::handleChordSelection() {
    // Implemented in renderChordList
}

void MidiChordTransformerApp::handleTransformationOptions() {
    // Implemented in renderControlPanel
}

void MidiChordTransformerApp::handleKeyDetection() {
    processor->detectKey();
}

void MidiChordTransformerApp::handleProgressionAnalysis() {
    processor->analyzeProgression();
}

void MidiChordTransformerApp::handleChordPreview() {
    // Implemented in renderChordList
}

void MidiChordTransformerApp::updateConsoleOutput(const std::string& message) {
    consoleOutput.push_back(message);
    
    // Limit console output size
    const size_t maxConsoleLines = 1000;
    if (consoleOutput.size() > maxConsoleLines) {
        consoleOutput.erase(consoleOutput.begin(), consoleOutput.begin() + (consoleOutput.size() - maxConsoleLines));
    }
}

void MidiChordTransformerApp::clearConsoleOutput() {
    consoleOutput.clear();
}

void MidiChordTransformerApp::initializeTransformationOptions() {
    // Create default transformation options
    auto defaultOptions = std::make_shared<TransformationOptions>();
    defaultOptions->type = TransformationType::STANDARD;
    defaultOptions->inversion = 0;
    defaultOptions->percentage = 100.0;
    defaultOptions->preserveRoot = true;
    defaultOptions->preserveBass = true;
    defaultOptions->useVoiceLeading = true;
    
    // Initialize with empty vectors
    selectedChords.clear();
    targetChordNames.clear();
    transformOptions.clear();
}

void MidiChordTransformerApp::resetChordSelection() {
    auto chords = processor->getChords();
    
    selectedChords.resize(chords.size(), false);
    targetChordNames.resize(chords.size());
    transformOptions.resize(chords.size());
    
    // Initialize transformation options
    for (size_t i = 0; i < chords.size(); i++) {
        if (!transformOptions[i]) {
            transformOptions[i] = std::make_shared<TransformationOptions>();
        }
        targetChordNames[i] = chords[i]->name;
    }
}

// ConsoleRedirector implementation
ConsoleRedirector::ConsoleRedirector(std::vector<std::string>& buffer)
    : outputBuffer(buffer), originalCoutBuffer(std::cout.rdbuf()) {
    // Redirect std::cout to our buffer
    // In a real implementation, this would use a custom streambuf
}

ConsoleRedirector::~ConsoleRedirector() {
    // Restore original buffer
    std::cout.rdbuf(originalCoutBuffer);
}

void ConsoleRedirector::write(const std::string& message) {
    outputBuffer.push_back(message);
    
    // Limit buffer size
    const size_t maxLines = 1000;
    if (outputBuffer.size() > maxLines) {
        outputBuffer.erase(outputBuffer.begin(), outputBuffer.begin() + (outputBuffer.size() - maxLines));
    }
}

} // namespace midi_transformer