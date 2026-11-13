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

## Editor capabilities

The editor supports durations from whole notes through 1/1024 with up to three dots; notes, triads, rests, ties, slurs, articulations, fermata variants, repeats, coda/segno markers, tempo, dynamic marks, crescendo/diminuendo, and niente wedges. Playback uses the 64-voice miniaudio piano synthesizer. Render modes include the grand staff, a pitch/octave honeycomb, Jianpu digits, and Japanese kanji. Bravura and a subsetted Noto Sans CJK JP font are bundled with their license notices.

The File menu supports versioned JSON project save/load, MusicXML import/export, and Standard MIDI File export. The current editor owns one measure, so file import/export is intentionally bounded to a single part and measure; it does not claim full multi-part or multi-measure score editing. MusicXML constructs outside the editor model may be simplified or rejected.

## Android APK signing

The Android workflow signs and verifies the APK before uploading the `note-studio-android-arm64-signed` artifact. Configure repository Actions secrets `ANDROID_KEYSTORE_BASE64`, `ANDROID_KEY_ALIAS`, `ANDROID_KEYSTORE_PASSWORD`, and `ANDROID_KEY_PASSWORD` to use a persistent signing identity suitable for releases and upgrades. If these secrets are absent, CI generates a temporary debug key for an installable test artifact; that fallback is not a stable release identity, so APKs signed by different runs will not update one another in place.

## Font licenses

Bravura is distributed under the SIL Open Font License; see `assets/OFL.txt`. The bundled Noto Sans CJK JP subset and its notice are in `assets/NotoSansCJKJP-subset.otf` and `assets/NotoSansCJK-OFL.txt`.
