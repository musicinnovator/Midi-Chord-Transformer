#!/bin/bash

# MIDI Chord Transformer Setup Script
# This script helps set up the project by downloading dependencies

echo "Setting up MIDI Chord Transformer..."

# Create directories if they don't exist
mkdir -p external/imgui
mkdir -p external/glfw
mkdir -p build

# Download ImGui
echo "Downloading ImGui..."
if [ ! -f external/imgui/imgui.h ]; then
    git clone --depth 1 https://github.com/ocornut/imgui.git temp_imgui
    cp temp_imgui/*.h external/imgui/
    cp temp_imgui/*.cpp external/imgui/
    rm -rf temp_imgui
    echo "ImGui downloaded successfully."
else
    echo "ImGui already exists, skipping download."
fi

# Download GLFW
echo "Downloading GLFW..."
if [ ! -d external/glfw/include ]; then
    # For a real project, you would download and build GLFW
    # For this example, we'll just create placeholder directories
    mkdir -p external/glfw/include/GLFW
    echo "// Placeholder GLFW header" > external/glfw/include/GLFW/glfw3.h
    echo "GLFW placeholder created."
    echo "Note: In a real project, you should download GLFW from https://www.glfw.org/"
else
    echo "GLFW directory already exists, skipping download."
fi

# Update CMakeLists.txt files for external dependencies
cat > external/imgui/CMakeLists.txt << EOF
cmake_minimum_required(VERSION 3.10)
project(imgui)

# Create ImGui library
add_library(imgui
    imgui.cpp
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    imgui_impl_glfw.cpp
    imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC \${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(imgui PUBLIC glfw \${OPENGL_LIBRARIES})
EOF

cat > external/glfw/CMakeLists.txt << EOF
cmake_minimum_required(VERSION 3.10)
project(glfw)

# In a real project, you would build GLFW from source or use find_package
# For this example, we'll create an interface library
add_library(glfw INTERFACE)
target_include_directories(glfw INTERFACE \${CMAKE_CURRENT_SOURCE_DIR}/include)
EOF

echo "Setup complete!"
echo "To build the project:"
echo "  cd build"
echo "  cmake .."
echo "  make"