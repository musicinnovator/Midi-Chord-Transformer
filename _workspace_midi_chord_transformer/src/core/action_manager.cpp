#include "../../include/core/action_manager.h"
#include "../../include/core/midi_processor.h"

#include <iostream>
#include <algorithm>

namespace midi_transformer {

ActionManager::ActionManager(MidiProcessor& proc) : processor(proc) {
    history = std::make_unique<ActionHistory>();
    history->currentPosition = 0;
    history->maxHistorySize = 50;
}

void ActionManager::recordTransformation(
    const std::vector<int>& indices,
    const std::vector<std::shared_ptr<Chord>>& before,
    const std::vector<std::shared_ptr<Chord>>& after,
    const std::string& description) {
    
    auto action = std::make_shared<TransformationAction>();
    action->type = TransformationAction::ActionType::TRANSFORM;
    action->affectedChordIndices = indices;
    
    // Convert shared_ptr<Chord> to Chord for storage
    action->previousState.reserve(before.size());
    for (const auto& chord : before) {
        action->previousState.push_back(*chord);
    }
    
    action->newState.reserve(after.size());
    for (const auto& chord : after) {
        action->newState.push_back(*chord);
    }
    
    action->description = description;
    action->timestamp = std::chrono::system_clock::now();
    
    // If we're not at the end of history, truncate
    if (history->currentPosition < history->actions.size()) {
        history->actions.resize(history->currentPosition);
    }
    
    // Add the action and update position
    history->actions.push_back(*action);
    history->currentPosition++;
    
    // Enforce max history size
    if (history->actions.size() > history->maxHistorySize) {
        history->actions.erase(history->actions.begin());
        history->currentPosition--;
    }
}

bool ActionManager::undo() {
    if (!canUndo()) {
        return false;
    }
    
    // Get the action to undo
    history->currentPosition--;
    const TransformationAction& action = history->actions[history->currentPosition];
    
    // Apply the undo
    for (size_t i = 0; i < action.affectedChordIndices.size(); i++) {
        int chordIndex = action.affectedChordIndices[i];
        
        if (chordIndex >= 0 && chordIndex < static_cast<int>(action.previousState.size())) {
            // Update the chord with its previous state
            processor.updateChord(chordIndex, action.previousState[i]);
        }
    }
    
    std::cout << "Undid: " << action.description << std::endl;
    return true;
}

bool ActionManager::redo() {
    if (!canRedo()) {
        return false;
    }
    
    // Get the action to redo
    const TransformationAction& action = history->actions[history->currentPosition];
    history->currentPosition++;
    
    // Apply the redo
    for (size_t i = 0; i < action.affectedChordIndices.size(); i++) {
        int chordIndex = action.affectedChordIndices[i];
        
        if (chordIndex >= 0 && chordIndex < static_cast<int>(action.newState.size())) {
            // Update the chord with its new state
            processor.updateChord(chordIndex, action.newState[i]);
        }
    }
    
    std::cout << "Redid: " << action.description << std::endl;
    return true;
}

bool ActionManager::canUndo() const {
    return history->currentPosition > 0;
}

bool ActionManager::canRedo() const {
    return history->currentPosition < history->actions.size();
}

std::string ActionManager::getUndoDescription() const {
    if (canUndo()) {
        return history->actions[history->currentPosition - 1].description;
    }
    return "Nothing to undo";
}

std::string ActionManager::getRedoDescription() const {
    if (canRedo()) {
        return history->actions[history->currentPosition].description;
    }
    return "Nothing to redo";
}

void ActionManager::clearHistory() {
    history->actions.clear();
    history->currentPosition = 0;
}

size_t ActionManager::getHistorySize() const {
    return history->actions.size();
}

} // namespace midi_transformer