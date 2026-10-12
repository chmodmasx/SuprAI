import QtQuick

// Safe read-only blockquote: "formatted" must come from the C++ allowlist.
Item {
    id: root
    objectName: "markdownQuote"

    property string formatted: ""
    signal externalLinkRequested(string url)

    implicitHeight: background.implicitHeight

    Rectangle {
        id: background
        width: root.width
        implicitHeight: Math.max(36, quoteText.implicitHeight + 24)
        radius: 7
        color: "#2a3037"

        Rectangle {
            width: 3
            height: parent.height
            color: "#51a2da"
            anchors.left: parent.left
            radius: 1
        }

        ProseText {
            id: quoteText
            anchors.left: parent.left
            anchors.leftMargin: 17
            anchors.top: parent.top
            anchors.topMargin: 12
            width: Math.max(0, parent.width - 29)
            formatted: root.formatted
            styled: true
            color: "#dbe5ed"
            onExternalLinkRequested: function(url) {
                root.externalLinkRequested(url)
            }
        }
    }
}
