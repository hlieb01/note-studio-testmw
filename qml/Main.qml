import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
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
    property int selectedDots: 0
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
            Button { text: "File"; onClicked: fileMenu.open() }
            Menu {
                id: fileMenu
                MenuItem { text: "Open project (JSON)…"; onTriggered: openJsonDialog.open() }
                MenuItem { text: "Save project (JSON)…"; onTriggered: saveJsonDialog.open() }
                MenuSeparator { }
                MenuItem { text: "Import MusicXML…"; onTriggered: importXmlDialog.open() }
                MenuItem { text: "Export MusicXML…"; onTriggered: exportXmlDialog.open() }
                MenuItem { text: "Export MIDI…"; onTriggered: exportMidiDialog.open() }
            }
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
            ComboBox {
                id: notationBox
                textRole: "label"
                model: [
                    { label: "Two Hands" },
                    { label: "Hexagonal" },
                    { label: "Jianpu" },
                    { label: "Japanese" }
                ]
                currentIndex: score.notationMode
                onActivated: score.setNotationMode(currentIndex)
            }
            Label { text: score.tempoName; color: "#9bbbc7" }
            SpinBox {
                from: 20
                to: 300
                value: score.tempoBpm
                onValueModified: score.setTempoBpm(value)
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
                notationMode: score.notationMode
                beats: score.beats
                beatUnit: score.beatUnit
                coda: score.coda
                segno: score.segno
                tempoBpm: score.tempoBpm
                dynamicMark: score.dynamicMark
                dynamicWedge: score.dynamicWedge
                notes: score.noteData
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 202
            radius: 16
            color: "#0b2332"
            border.color: "#173d50"
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10
                Flickable {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 54
                    clip: true
                    contentWidth: pitchDurationTools.implicitWidth
                    contentHeight: height
                    RowLayout {
                    id: pitchDurationTools
                    height: parent.height
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
                        model: [1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024]
                        delegate: Button {
                            required property int modelData
                            text: "1/" + modelData
                            checkable: true
                            checked: window.selectedDuration === modelData
                            onClicked: window.selectedDuration = modelData
                        }
                    }
                    ComboBox {
                        id: dotsBox
                        model: ["No dot", "·", "··", "···"]
                        currentIndex: window.selectedDots
                        onActivated: window.selectedDots = currentIndex
                    }
                    }
                }
                Flickable {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 58
                    clip: true
                    contentWidth: symbolTools.implicitWidth
                    contentHeight: height
                    RowLayout {
                    id: symbolTools
                    height: parent.height
                    Label { text: "SYMBOLS"; color: "#75eee5"; font.bold: true; font.pixelSize: 11 }
                    Button { text: "♪ Note"; onClicked: score.addNote(window.selectedMidi, window.selectedDuration, window.selectedDots) }
                    Button { text: "♩ Chord"; onClicked: score.addChord(window.selectedMidi, window.selectedDuration, window.selectedDots) }
                    Button { text: "𝄽 Rest"; onClicked: score.addRest(window.selectedDuration, window.selectedDots) }
                    ComboBox {
                        id: fermataBox
                        model: ["Fermata", "Short", "Long", "Very long", "Triangle", "Square"]
                    }
                    Button { text: "Apply"; enabled: score.noteCount > 0; onClicked: score.addFermata(fermataBox.currentIndex) }
                    Button { text: "Tie"; enabled: score.noteCount > 1; onClicked: score.tieLastTwo() }
                    Button { text: "Slur"; enabled: score.noteCount > 1; onClicked: score.slurLastTwo() }
                    Button { text: "Repeat"; onClicked: score.toggleRepeat() }
                    Button { text: "𝄌 Coda"; onClicked: score.toggleCoda() }
                    Button { text: "𝄋 Segno"; onClicked: score.toggleSegno() }
                    ComboBox {
                        id: dynamicBox
                        model: ["ppp", "pp", "p", "mp", "mf", "f", "ff", "fff", "niente"]
                        currentIndex: score.dynamicMark
                        onActivated: score.setDynamics(currentIndex, wedgeBox.currentIndex)
                    }
                    ComboBox {
                        id: wedgeBox
                        model: ["No wedge", "Crescendo", "Diminuendo", "Cresc. niente", "Dim. niente"]
                        currentIndex: score.dynamicWedge
                        onActivated: score.setDynamics(dynamicBox.currentIndex, currentIndex)
                    }
                    ComboBox {
                        id: articulationBox
                        model: ["None", "Staccato", "Accent", "Sforzando"]
                        onActivated: score.setLastArticulation(["none", "staccato", "accent", "sforzando"][currentIndex])
                    }
                    Item { Layout.fillWidth: true }
                    Button { text: "Clear"; onClicked: score.clearMeasure() }
                    Button {
                        id: playbackButton
                        text: score.playing ? "■  Stop" : "▶  Play"
                        highlighted: true
                        implicitWidth: 150
                        implicitHeight: 58
                        font.pixelSize: 20
                        font.bold: true
                        onClicked: score.togglePlayback()
                    }
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

    FileDialog {
        id: openJsonDialog
        title: "Open Note Studio project"
        fileMode: FileDialog.OpenFile
        nameFilters: ["Note Studio project (*.json)", "All files (*)"]
        onAccepted: score.loadProjectJson(selectedFile.toLocalFile())
    }
    FileDialog {
        id: saveJsonDialog
        title: "Save Note Studio project"
        fileMode: FileDialog.SaveFile
        currentFile: "note-studio-project.json"
        nameFilters: ["Note Studio project (*.json)", "All files (*)"]
        onAccepted: score.saveProjectJson(selectedFile.toLocalFile())
    }
    FileDialog {
        id: importXmlDialog
        title: "Import MusicXML"
        fileMode: FileDialog.OpenFile
        nameFilters: ["MusicXML (*.musicxml *.xml)", "All files (*)"]
        onAccepted: score.importMusicXml(selectedFile.toLocalFile())
    }
    FileDialog {
        id: exportXmlDialog
        title: "Export MusicXML"
        fileMode: FileDialog.SaveFile
        currentFile: "score.musicxml"
        nameFilters: ["MusicXML (*.musicxml)", "All files (*)"]
        onAccepted: score.exportMusicXml(selectedFile.toLocalFile())
    }
    FileDialog {
        id: exportMidiDialog
        title: "Export MIDI"
        fileMode: FileDialog.SaveFile
        currentFile: "score.mid"
        nameFilters: ["MIDI file (*.mid)", "All files (*)"]
        onAccepted: score.exportMidi(selectedFile.toLocalFile())
    }
}
