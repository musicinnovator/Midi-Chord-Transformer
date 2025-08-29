# Placeholder CMakeLists.txt for ImGui
# In a real project, you would download ImGui from https://github.com/ocornut/imgui

cmake_minimum_required(VERSION 3.10)
project(imgui)

add_library(imgui INTERFACE)
target_include_directories(imgui INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Note: This is just a placeholder. In a real project, you would need to:
# 1. Download ImGui source files
# 2. Add them to the library
# 3. Configure proper compilation settings

message(STATUS "Note: This is a placeholder for ImGui. Please download the actual library.")