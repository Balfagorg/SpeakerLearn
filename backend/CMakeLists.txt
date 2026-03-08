cmake_minimum_required(VERSION 3.20)
project(SpeakerLearn VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Dependencies ---
include(FetchContent)

# 1. cpp-httplib (HTTP API Server)
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.15.3
)
FetchContent_MakeAvailable(httplib)

# 2. nlohmann_json (JSON Parsing)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(json)

# 3. sqlite3 (Database)
# On many systems sqlite3 is available natively. For true cross-platform portability without
# assuming it's installed, we use a cmake-enabled sqlite3 amalgamation.
FetchContent_Declare(
    sqlite3
    GIT_REPOSITORY https://github.com/azadkuh/sqlite-amalgamation.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(sqlite3)

# --- Platform Specific Directories ---
set(PLATFORM_SRC "")
if(WIN32)
    message(STATUS "Configuring for Windows...")
    file(GLOB PLATFORM_SRC "src/platform/windows/*.cpp")
elseif(APPLE)
    message(STATUS "Configuring for macOS...")
    file(GLOB PLATFORM_SRC "src/platform/macos/*.cpp" "src/platform/macos/*.mm")
else()
    message(STATUS "Configuring for Linux...")
    file(GLOB PLATFORM_SRC "src/platform/linux/*.cpp")
endif()

# --- Source Files ---
file(GLOB_RECURSE CORE_SRC "src/core/*.cpp")
file(GLOB_RECURSE DB_SRC "src/db/*.cpp")
file(GLOB_RECURSE API_SRC "src/api/*.cpp")

# Note: We will add the main.cpp or library targets below once we write them.
set(ALL_SRC 
    ${CORE_SRC}
    ${DB_SRC}
    ${API_SRC}
    ${PLATFORM_SRC}
)

# For now we create a library for the core system, to make it linkable into mobile apps or tests.
add_library(speakerlearn_core STATIC ${ALL_SRC})
target_include_directories(speakerlearn_core PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/platform/include>
    ${sqlite3_SOURCE_DIR}
)

target_link_libraries(speakerlearn_core PUBLIC
    httplib::httplib
    nlohmann_json::nlohmann_json
    SQLite3
)

# Core DSP engine links here (assuming it remains separate or we pull it into src/core later).
# We'll pull it in or build it later in the process.

# --- Executable for Desktop Serving/Testing ---
add_executable(speakerlearn_serverd src/main.cpp)
target_link_libraries(speakerlearn_serverd PRIVATE speakerlearn_core)

# --- Install Targets ---
install(TARGETS speakerlearn_serverd DESTINATION bin)

# --- CPack Configuration ---
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_NAME "SpeakerLearn")
set(CPACK_PACKAGE_VENDOR "Balfagorg")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Adaptive Audio Balancing Platform C++ Node")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
elseif(APPLE)
    set(CPACK_GENERATOR "ZIP;DragNDrop")
else()
    set(CPACK_GENERATOR "TGZ;DEB")
endif()

include(CPack)
