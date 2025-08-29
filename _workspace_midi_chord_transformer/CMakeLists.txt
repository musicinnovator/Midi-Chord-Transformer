cmake_minimum_required(VERSION 3.10)
project(MidiChordTransformer VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/external)

# Add external dependencies
# Note: In a real project, you would need to download or include these libraries
# For this example, we assume they are available in the 'external' directory
add_subdirectory(external/imgui)
add_subdirectory(external/glfw)

# Source files
set(CORE_SOURCES
    src/core/midi_processor.cpp
    src/core/voice_leading_engine.cpp
    src/core/chord_progression_analyzer.cpp
    src/core/key_detector.cpp
    src/core/chord_synthesizer.cpp
    src/core/action_manager.cpp
)

set(GUI_SOURCES
    src/gui/midi_chord_transformer_app.cpp
)

set(UTILS_SOURCES
    src/utils/midi_utils.cpp
)

# Main executable
add_executable(midi_chord_transformer
    src/main.cpp
    ${CORE_SOURCES}
    ${GUI_SOURCES}
    ${UTILS_SOURCES}
)

# Link libraries
target_link_libraries(midi_chord_transformer
    imgui
    glfw
    ${OPENGL_LIBRARIES}
)

# Install target
install(TARGETS midi_chord_transformer DESTINATION bin)

# Create directories for external dependencies
file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/external/imgui)
file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/external/glfw)

# Add a message to remind users to download dependencies
add_custom_command(TARGET midi_chord_transformer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Note: You need to download ImGui and GLFW libraries to the 'external' directory before building."
)

# Enable testing
enable_testing()

# Add tests directory if it exists
if(EXISTS ${PROJECT_SOURCE_DIR}/tests)
    add_subdirectory(tests)
endif()