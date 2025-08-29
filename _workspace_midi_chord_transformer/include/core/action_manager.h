#pragma once

#include "midi_structures.h"
#include <vector>
#include <string>
#include <memory>
#include <chrono>

namespace midi_transformer {

// Forward declaration
class MidiProcessor;

// For tracking user actions for undo/redo functionality
struct TransformationAction {
    enum class ActionType { TRANSFORM, REVERT, BATCH_TRANSFORM };
    ActionType type;
    std::vector<int> affectedChordIndices;
    std::vector<Chord> previousState;
    std::vector<Chord> newState;
    std::string description;        // Human-readable description
    std::chrono::system_clock::time_point timestamp;
};

// For managing action history
struct ActionHistory {
    std::vector<TransformationAction> actions;
    size_t currentPosition;         // For undo/redo navigation
    size_t maxHistorySize;          // Limit history size
};

class ActionManager {
private:
    std::unique_ptr<ActionHistory> history;
    MidiProcessor& processor;
    
public:
    ActionManager(MidiProcessor& proc);
    
    void recordTransformation(
        const std::vector<int>& indices,
        const std::vector<std::shared_ptr<Chord>>& before,
        const std::vector<std::shared_ptr<Chord>>& after,
        const std::string& description);
    
    bool undo();
    bool redo();
    
    bool canUndo() const;
    bool canRedo() const;
    
    std::string getUndoDescription() const;
    std::string getRedoDescription() const;
    
    void clearHistory();
    size_t getHistorySize() const;
};

} // namespace midi_transformer