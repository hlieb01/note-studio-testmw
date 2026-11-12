#include "ui/hex_renderer.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QUrl>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    qmlRegisterType<notestudio::ui::HexRenderer>("NoteStudio", 1, 0, "HexRenderer");
    QQmlApplicationEngine engine;
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
