# Note Studio

C++20 / Qt 6 Quick notation editor for Windows (MSVC) and Android (NDK). The core score model is Qt-independent. `ScoreController` connects QML editing and playback to a 64-voice mathematical piano synthesizer. The default renderer draws a grand staff using the embedded Bravura SMuFL font; the concentric-hexagon view remains an optional visual mode.

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

The model supports notes, rests, chords, dotted durations through 1/512, arbitrary power-of-two beat units, fermatas, ties, slurs, articulations, repeat markers and coda metadata. The editor schedules the current measure at 120 BPM and sends note-on/note-off commands to miniaudio. The first chord control inserts a major triad. The renderer draws a treble/bass grand staff by default and has an optional hexagonal easter-egg mode.

Bravura is the SMuFL reference font and is distributed under the SIL Open Font License; the license text is included at `assets/OFL.txt`.
