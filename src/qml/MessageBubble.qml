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
                CopyFeedbackButton {
                    copyLabel: "Copiar respuesta"
                    stableWidth: 132
                    contentToCopy: root.text
                    onCopyRequested: function(value) { root.copyRequested(value) }
                }
            }

            Repeater {
                model: root.messageBlocks && root.messageBlocks.length > 0
                       ? root.messageBlocks
                       : [{ "kind": "text", "formatted": root.displayText }]

                Loader {
                    id: segmentLoader
                    required property var modelData
                    required property int index
                    property var segment: modelData
                    property bool isLastSegment: index === (
                        root.messageBlocks && root.messageBlocks.length > 0
                        ? root.messageBlocks.length - 1 : 0)
                    width: content.width
                    height: item ? item.implicitHeight : 0
                    sourceComponent: modelData.kind === "code" ? codeSegment
                                     : modelData.kind === "table" ? tableSegment
                                     : modelData.kind === "quote" ? quoteSegment
                                     : proseSegment
                }
            }

            // Prose owns its own cursor; structured tails show one below
            // the last table/quote/code component, never after every block.
            Text {
                visible: root.streaming && root.messageBlocks
                         && root.messageBlocks.length > 0
                         && root.messageBlocks[root.messageBlocks.length - 1].kind !== "text"
                text: "▋"
                font.pixelSize: 14
                color: root.foregroundColor
            }
        }
    }

    Component {
        id: proseSegment

        ProseText {
            width: parent.width
            formatted: parent.segment.formatted
            styled: (root.messageBlocks && root.messageBlocks.length > 0)
                    || root.displayStyled
            streaming: root.streaming && parent.isLastSegment
            messageState: root.messageState && parent.isLastSegment
                          ? root.messageState : ""
            color: root.foregroundColor
            onExternalLinkRequested: function(url) { root.externalLinkRequested(url) }
        }
    }

    Component {
        id: tableSegment

        MarkdownTable {
            width: parent.width
            tableData: parent.segment
            onCopyRequested: function(value) { root.copyRequested(value) }
            onExternalLinkRequested: function(url) { root.externalLinkRequested(url) }
        }
    }

    Component {
        id: quoteSegment

        MarkdownQuote {
            width: parent.width
            formatted: parent.segment.formatted
            onExternalLinkRequested: function(url) { root.externalLinkRequested(url) }
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
                    CopyFeedbackButton {
                        stableWidth: 94
                        contentToCopy: codeCard.segmentData.raw
                        onCopyRequested: function(value) { root.copyRequested(value) }
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
