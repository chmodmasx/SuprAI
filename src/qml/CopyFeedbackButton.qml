import QtQuick
import QtQuick.Controls

// Reusable clipboard feedback. Fixed dimensions avoid moving surrounding
// controls when the label changes, and reset on delegate reuse/new content.
Button {
    id: root
    objectName: "copyFeedbackButton"

    property string copyLabel: "Copiar"
    property string contentToCopy: ""
    property int stableWidth: 92
    property bool copied: false

    signal copyRequested(string content)

    implicitWidth: stableWidth
    implicitHeight: 25
    font.pixelSize: 11
    text: copied ? "✓ Copiado" : copyLabel

    function confirmCopy() {
        copied = true
        resetFeedback.restart()
    }

    onContentToCopyChanged: {
        resetFeedback.stop()
        copied = false
    }

    Timer {
        id: resetFeedback
        interval: 1800
        repeat: false
        onTriggered: root.copied = false
    }

    onClicked: {
        root.copyRequested(root.contentToCopy)
        root.confirmCopy()
    }

    background: Rectangle {
        radius: 6
        color: root.down ? "#435061" : root.hovered ? "#3b4655" : "#35383d"
        border.color: root.copied ? "#51A2DA" : "#555b64"
    }

    contentItem: Text {
        text: root.text
        color: "#f6f6f6"
        font: root.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
