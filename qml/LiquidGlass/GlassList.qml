import QtQuick
import QtQuick.Controls.Basic

/*
    GlassList — ListView tuned for glass cards: clipped, bounded,
    with a thin unobtrusive scrollbar. Supply model + delegate as usual;
    pair with GlassListRow for delegates.
*/
ListView {
    clip: true
    boundsBehavior: Flickable.StopAtBounds

    ScrollBar.vertical: ScrollBar {
        policy: ScrollBar.AsNeeded
        contentItem: Rectangle {
            implicitWidth: 4
            radius: 2
            color: Qt.rgba(1, 1, 1, 0.25)
        }
    }
}
