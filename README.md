# Note Studio

C++20 / Qt 6 Quick starter for Windows (MSVC) and Android (NDK). The core score model is Qt-independent; the piano engine synthesizes tones mathematically, and the custom `HexRenderer` draws the staff as concentric hexagons.

## Requirements

- CMake 3.21+
- Qt 6.5+ with Quick and QML (plus an Android Qt kit for Android builds)
- C++20 compiler; MSVC on Windows
- Internet access at configure time for CMake FetchContent to retrieve miniaudio 0.11.21

## Build

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/<kit>
cmake --build build --config Release
```

For Android, configure with the Qt-provided Android toolchain and SDK/NDK settings, for example via Qt Creator or the official Qt Android CMake preset/toolchain. For Windows, use a Visual Studio generator and a Qt MSVC kit matching the selected architecture.

## Current starter scope

The score types support pitches, dotted durations, notes/rests/chords and common meter metadata (including 4/4 and 6/8) plus repeat markers. `PianoSynth` exposes note-on/note-off and initializes a stereo float-output miniaudio device. The QML Play/Stop button currently toggles the UI preview state; connecting score playback scheduling and synthesizer control is the next integration step.
