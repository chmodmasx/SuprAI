import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    required property string itemId
    required property string speaker
    required property string text
    required property string displayText
    required property bool displayStyled
    required property bool streaming
    required property string messageState
    required property var messageBlocks

    property color foregroundColor: "#f6f6f6"
    property color mutedColor: "#afafaf"
    property color panelColor: "#313131"
    property color borderColor: "#4e4e4e"
    property color accentColor: "#51a2da"

    signal copyRequested(string content)
    signal externalLinkRequested(string url)

    implicitHeight: bubble.implicitHeight

    Rectangle {
        id: bubble
        width: Math.min(root.width * 0.84, 780)
        implicitHeight: content.implicitHeight + 26
        radius: 12
        anchors.right: root.speaker === "user" ? parent.right : undefined
        anchors.left: root.speaker === "user" ? undefined : parent.left
        color: root.speaker === "user" ? "#24445A" : root.panelColor
        border.color: root.speaker === "user" ? root.accentColor : root.borderColor
        border.width: 1

        Column {
            id: content
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 13
            spacing: 8

            Row {
                width: parent.width
                height: root.speaker === "assistant" && root.messageState === "completed" ? 22 : 0
                visible: height > 0
                layoutDirection: Qt.RightToLeft
                Button {
                    text: "Copiar respuesta"
                    height: 22
                    font.pixelSize: 11
                    onClicked: root.copyRequested(root.text)
                }
            }

            Repeater {
                model: root.messageBlocks && root.messageBlocks.length > 0
                       ? root.messageBlocks
                       : [{ "kind": "text", "formatted": root.displayText }]

                Loader {
                    id: segmentLoader
                    required property var modelData
                    property var segment: modelData
                    width: content.width
                    height: item ? item.implicitHeight : 0
                    sourceComponent: modelData.kind === "code" ? codeSegment : proseSegment
                }
            }
        }
    }

    Component {
        id: proseSegment

        TextEdit {
            width: parent.width
            text: parent.segment.formatted
                  + (root.streaming ? " ▋"
                     : root.messageState === "cancelled" ? " (cancelado)"
                     : root.messageState === "failed" ? " (error)" : "")
            textFormat: root.messageBlocks && root.messageBlocks.length > 0
                        ? TextEdit.StyledText
                        : root.displayStyled ? TextEdit.StyledText : TextEdit.PlainText
            color: root.foregroundColor
            font.pixelSize: 14
            lineHeight: 1.25
            readOnly: true
            selectByMouse: true
            wrapMode: TextEdit.Wrap
            onLinkActivated: function(url) { root.externalLinkRequested(url) }
        }
    }

    Component {
        id: codeSegment

        Rectangle {
            id: codeCard
            property var segmentData: parent.segment
            width: parent.width
            color: "#24272b"
            border.color: root.borderColor
            border.width: 1
            radius: 8
            implicitHeight: codeColumn.implicitHeight + 16

            Column {
                id: codeColumn
                width: parent.width - 20
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.margins: 10
                spacing: 7

                RowLayout {
                    width: parent.width
                    height: 26
                    Label {
                        Layout.fillWidth: true
                        text: codeCard.segmentData.language || "Código"
                        color: root.mutedColor
                        font.pixelSize: 12
                        elide: Text.ElideRight
                    }
                    Button {
                        text: "Copiar código"
                        font.pixelSize: 11
                        onClicked: root.copyRequested(codeCard.segmentData.raw)
                    }
                }

                ScrollView {
                    width: parent.width
                    height: Math.min(320, Math.max(24, codeText.implicitHeight + 8))
                    clip: true
                    ScrollBar.horizontal.policy: ScrollBar.AsNeeded
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded

                    TextEdit {
                        id: codeText
                        width: Math.max(implicitWidth, parent.width)
                        text: codeCard.segmentData.raw
                        textFormat: TextEdit.PlainText
                        readOnly: true
                        selectByMouse: true
                        wrapMode: TextEdit.NoWrap
                        color: root.foregroundColor
                        font.family: "monospace"
                        font.pixelSize: 13
                    }
                }
            }
        }
    }
}
