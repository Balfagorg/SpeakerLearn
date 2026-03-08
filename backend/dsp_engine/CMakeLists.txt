cmake_minimum_required(VERSION 3.10)

project(SpeakerLearnDSP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compile as a shared library so Python can load it via ctypes
add_library(speakerlearn_dsp SHARED engine.cpp)

# Optional: Set output directory for the compiled DLL/SO to be alongside python code
set_target_properties(speakerlearn_dsp PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../core
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../core
)
