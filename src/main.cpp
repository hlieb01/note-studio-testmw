#include "ui/hex_renderer.hpp"
#include "ui/score_controller.hpp"

#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    qmlRegisterType<notestudio::ui::HexRenderer>("NoteStudio", 1, 0, "HexRenderer");
    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/assets/Bravura.otf"));
    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/assets/NotoSansCJKJP-subset.otf"));
    notestudio::ui::ScoreController score;
    score.initializeAudio();
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("score"), &score);
    const QUrl mainUrl(QStringLiteral("qrc:/qt/qml/NoteStudio/qml/Main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app,
                     [mainUrl](QObject* object, const QUrl& objectUrl) {
                         if (object == nullptr && objectUrl == mainUrl) {
                             QCoreApplication::exit(EXIT_FAILURE);
                         }
                     }, Qt::QueuedConnection);
    engine.load(mainUrl);
    return app.exec();
}
