cmake_minimum_required(VERSION 3.21)
project(NoteStudio VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
find_package(Qt6 6.5 REQUIRED COMPONENTS Quick Qml)
qt_standard_project_setup(REQUIRES 6.5)

# miniaudio is header-only; FetchContent makes the dependency reproducible.
FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG 0.11.21
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(miniaudio)

qt_add_executable(note-studio
    src/main.cpp
    src/core/score.hpp
    src/core/score.cpp
    src/audio/piano_synth.hpp
    src/audio/piano_synth.cpp
    src/ui/hex_renderer.hpp
    src/ui/hex_renderer.cpp
    src/ui/score_controller.hpp
    src/ui/score_controller.cpp
)

qt_add_qml_module(note-studio
    URI NoteStudio
    VERSION 1.0
    QML_FILES qml/Main.qml
)

qt_add_resources(note-studio notation_fonts
    PREFIX "/fonts"
    FILES assets/Bravura.otf assets/OFL.txt
)

target_include_directories(note-studio PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${miniaudio_SOURCE_DIR}
)
target_link_libraries(note-studio PRIVATE Qt6::Quick Qt6::Qml)
target_compile_definitions(note-studio PRIVATE MA_NO_DECODING MA_NO_ENCODING MA_NO_RESOURCE_MANAGER)

if(MSVC)
    target_compile_options(note-studio PRIVATE /W4 /permissive-)
else()
    target_compile_options(note-studio PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()

if(NOT ANDROID)
    install(TARGETS note-studio RUNTIME DESTINATION bin)
endif()
