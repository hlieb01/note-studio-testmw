import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import NoteStudio 1.0

ApplicationWindow {
    id: window
    width: 1180
    height: 780
    minimumWidth: 800
    minimumHeight: 620
    visible: true
    title: "Note Studio"
    color: "#061724"
    property int selectedDuration: 4
    property int selectedMidi: 60
    readonly property var pitchValues: [60, 62, 64, 65, 67, 69, 71, 72]

    header: ToolBar {
        background: Rectangle { color: "#0b2332" }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            Label { text: "Note Studio"; color: "#d9f7fa"; font.pixelSize: 24; font.bold: true }
            Label { text: "· Piano score"; color: "#80a8b8"; font.pixelSize: 14 }
            Item { Layout.fillWidth: true }
            Label { text: "Meter"; color: "#9bbbc7" }
            ComboBox {
                id: meterBox
                textRole: "label"
                model: [
                    { label: "4/4", beats: 4, unit: 4 },
                    { label: "3/4", beats: 3, unit: 4 },
                    { label: "6/8", beats: 6, unit: 8 },
                    { label: "7/8", beats: 7, unit: 8 },
                    { label: "4/8", beats: 4, unit: 8 }
                ]
                onActivated: function(index) {
                    const signature = model[index]
                    score.setTimeSignature(signature.beats, signature.unit)
                }
            }
            Switch {
                text: checked ? "Hex" : "Staff"
                checked: score.hexMode
                onToggled: score.hexMode = checked
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 14

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 16
            color: "#081d2b"
            border.color: "#173d50"
            HexRenderer {
                id: staff
                anchors.fill: parent
                anchors.margins: 12
                hexMode: score.hexMode
                beats: score.beats
                beatUnit: score.beatUnit
                coda: score.coda
                notes: score.noteData
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 184
            radius: 16
            color: "#0b2332"
            border.color: "#173d50"
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "PITCH"; color: "#75eee5"; font.bold: true; font.pixelSize: 11 }
                    Repeater {
                        model: ["C4", "D4", "E4", "F4", "G4", "A4", "B4", "C5"]
                        delegate: Button {
                            required property string modelData
                            text: modelData
                            checkable: true
                            checked: window.selectedMidi === window.pitchValues[index]
                            onClicked: window.selectedMidi = window.pitchValues[index]
                        }
                    }
                    Item { Layout.fillWidth: true }
                    Label { text: "DURATION"; color: "#75eee5"; font.bold: true; font.pixelSize: 11 }
                    Repeater {
                        model: [1, 2, 4, 8, 16, 32, 64, 128, 256, 512]
                        delegate: Button {
                            required property int modelData
                            text: "1/" + modelData
                            checkable: true
                            checked: window.selectedDuration === modelData
                            onClicked: window.selectedDuration = modelData
                        }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "SYMBOLS"; color: "#75eee5"; font.bold: true; font.pixelSize: 11 }
                    Button { text: "♪ Note"; onClicked: score.addNote(window.selectedMidi, window.selectedDuration) }
                    Button { text: "♩ Chord"; onClicked: score.addChord(window.selectedMidi, window.selectedDuration) }
                    Button { text: "𝄽 Rest"; onClicked: score.addRest(window.selectedDuration) }
                    Button { text: "⌒ Fermata"; enabled: score.noteCount > 0; onClicked: score.addFermata() }
                    Button { text: "Tie"; enabled: score.noteCount > 1; onClicked: score.tieLastTwo() }
                    Button { text: "Slur"; enabled: score.noteCount > 1; onClicked: score.slurLastTwo() }
                    Button { text: "Repeat"; onClicked: score.toggleRepeat() }
                    Button { text: "𝄌 Coda"; onClicked: score.toggleCoda() }
                    ComboBox {
                        id: articulationBox
                        model: ["None", "Staccato", "Accent", "Sforzando"]
                        onActivated: score.setLastArticulation(["none", "staccato", "accent", "sforzando"][currentIndex])
                    }
                    Item { Layout.fillWidth: true }
                    Button { text: "Clear"; onClicked: score.clearMeasure() }
                    Button {
                        text: score.playing ? "■  Stop" : "▶  Play"
                        highlighted: true
                        onClicked: score.togglePlayback()
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: score.message; color: "#9bbbc7"; Layout.fillWidth: true }
                    Label { text: score.noteCount + " event(s) · " + score.beats + "/" + score.beatUnit; color: "#779aaa" }
                }
            }
        }
    }
}
