#pragma once

#include "score.hpp"

#include <QString>

namespace notestudio::core {

class ScoreIO final {
public:
    static bool saveJson(const Measure& measure, const QString& path, QString* error = nullptr);
    static bool loadJson(Measure& measure, const QString& path, QString* error = nullptr);
    static bool exportMusicXml(const Measure& measure, const QString& path, QString* error = nullptr);
    static bool importMusicXml(Measure& measure, const QString& path, QString* error = nullptr);
    static bool exportMidi(const Measure& measure, const QString& path, int bpm = 120, QString* error = nullptr);
};

} // namespace notestudio::core
