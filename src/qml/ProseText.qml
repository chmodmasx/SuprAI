import QtQuick

// Selectable read-only view for C++-generated allowlisted HTML. Never receives
// raw model HTML/Markdown when "styled" is true. No WebEngine or image tags.
TextEdit {
    id: root
    objectName: "proseText"

    property string formatted: ""
    property bool styled: false
    property bool streaming: false
    property string messageState: ""

    signal externalLinkRequested(string url)

    text: formatted
          + (streaming ? " ▋"
             : messageState === "cancelled" ? " (cancelado)"
             : messageState === "failed" ? " (error)" : "")
    textFormat: styled ? TextEdit.RichText : TextEdit.PlainText
    color: "#f6f6f6"
    font.pixelSize: 14
    readOnly: true
    selectByMouse: true
    selectByKeyboard: true
    wrapMode: TextEdit.Wrap

    onLinkActivated: function(url) { root.externalLinkRequested(url) }
}
