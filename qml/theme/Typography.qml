pragma Singleton

import QtQuick

QtObject {
    id: root

    component FontRole: QtObject {
        required property string family
        required property int pixelSize
        required property int weight
        required property real letterSpacing
    }

    readonly property FontLoader jetBrainsRegular: FontLoader {
        source: "qrc:/fonts/JetBrainsMono-Regular.ttf"
    }
    readonly property FontLoader jetBrainsMedium: FontLoader {
        source: "qrc:/fonts/JetBrainsMono-Medium.ttf"
    }
    readonly property FontLoader jetBrainsBold: FontLoader {
        source: "qrc:/fonts/JetBrainsMono-Bold.ttf"
    }
    readonly property FontLoader interRegular: FontLoader {
        source: "qrc:/fonts/Inter-Regular.ttf"
    }
    readonly property FontLoader interMedium: FontLoader {
        source: "qrc:/fonts/Inter-Medium.ttf"
    }
    readonly property FontLoader interSemiBold: FontLoader {
        source: "qrc:/fonts/Inter-SemiBold.ttf"
    }
    readonly property FontLoader interBold: FontLoader {
        source: "qrc:/fonts/Inter-Bold.ttf"
    }

    readonly property string monoFamily: jetBrainsRegular.status === FontLoader.Ready ? jetBrainsRegular.name : "JetBrains Mono"
    readonly property string sansFamily: interRegular.status === FontLoader.Ready ? interRegular.name : "Inter"

    readonly property FontRole display: FontRole {
        family: root.jetBrainsBold.status === FontLoader.Ready ? root.jetBrainsBold.name : "JetBrains Mono"
        pixelSize: 32
        weight: Font.Bold
        letterSpacing: 0
    }
    readonly property FontRole data: FontRole {
        family: root.jetBrainsMedium.status === FontLoader.Ready ? root.jetBrainsMedium.name : "JetBrains Mono"
        pixelSize: 15
        weight: Font.Medium
        letterSpacing: 0
    }
    readonly property FontRole dataSmall: FontRole {
        family: root.jetBrainsMedium.status === FontLoader.Ready ? root.jetBrainsMedium.name : "JetBrains Mono"
        pixelSize: 12
        weight: Font.Medium
        letterSpacing: 0.5
    }
    readonly property FontRole title: FontRole {
        family: root.interSemiBold.status === FontLoader.Ready ? root.interSemiBold.name : "Inter"
        pixelSize: 20
        weight: Font.DemiBold
        letterSpacing: 0
    }
    readonly property FontRole heading: FontRole {
        family: root.interSemiBold.status === FontLoader.Ready ? root.interSemiBold.name : "Inter"
        pixelSize: 15
        weight: Font.DemiBold
        letterSpacing: 0
    }
    readonly property FontRole label: FontRole {
        family: root.interMedium.status === FontLoader.Ready ? root.interMedium.name : "Inter"
        pixelSize: 13
        weight: Font.Medium
        letterSpacing: 0.2
    }
    readonly property FontRole body: FontRole {
        family: root.interRegular.status === FontLoader.Ready ? root.interRegular.name : "Inter"
        pixelSize: 14
        weight: Font.Normal
        letterSpacing: 0
    }
    readonly property FontRole caption: FontRole {
        family: root.interMedium.status === FontLoader.Ready ? root.interMedium.name : "Inter"
        pixelSize: 11
        weight: Font.Medium
        letterSpacing: 0.4
    }
}
