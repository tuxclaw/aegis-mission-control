import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 1280
    height: 800
    minimumWidth: 1024
    minimumHeight: 720
    visible: true
    title: qsTr("AEGIS Mission Control")
    color: "#0a0e1a"

    Label {
        anchors.centerIn: parent
        text: qsTr("AEGIS")
        color: "#00d4ff"
        font.family: "Inter"
        font.pixelSize: 48
        font.weight: Font.DemiBold
    }
}
