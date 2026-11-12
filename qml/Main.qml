import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import NoteStudio 1.0

ApplicationWindow {
    id: window
    width: 1100
    height: 720
    minimumWidth: 760
    minimumHeight: 520
    visible: true
    title: "Note Studio"
    color: "#061724"

    property bool playing: false

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 92
            color: "#0b2332"
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 12
                Label { text: "NOTE"; color: "#76eee5"; font.bold: true; font.pixelSize: 15 }
                Repeater {
                    model: ["♪", "♫", "♩", "𝄽", "♯"]
                    delegate: Rectangle {
                        required property string modelData
                        Layout.preferredWidth: 60
                        Layout.preferredHeight: 54
                        radius: 12
                        color: index === 0 ? "#164b5b" : "#102e3e"
                        border.color: "#246276"
                        Label { anchors.centerIn: parent; text: modelData; color: "#b8fff6"; font.pixelSize: 24 }
                    }
                }
                Item { Layout.fillHeight: true }
                Label { text: "4/4"; color: "#88aebd"; font.pixelSize: 13; Layout.alignment: Qt.AlignHCenter }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 22
            spacing: 16

            RowLayout {
                Layout.fillWidth: true
                Label { text: "Note Studio"; color: "#d9f7fa"; font.pixelSize: 26; font.bold: true }
                Item { Layout.fillWidth: true }
                Label { text: "Piano · 64 voices"; color: "#7299a9"; font.pixelSize: 13 }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 18
                color: "#081d2b"
                border.color: "#173d50"
                HexRenderer { anchors.fill: parent; anchors.margins: 8 }
            }

            RowLayout {
                Layout.fillWidth: true
                Button {
                    text: window.playing ? "■  Stop" : "▶  Play"
                    highlighted: true
                    onClicked: window.playing = !window.playing
                }
                Label { text: window.playing ? "Playback preview" : "Ready"; color: "#86abb9"; leftPadding: 10 }
                Item { Layout.fillWidth: true }
                Label { text: "120 BPM"; color: "#a7c8d2" }
            }
        }
    }
}
