import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Native, bounded Markdown table. All rich text comes from C++ sanitization;
// the table never loads resources or executes model content.
Item {
    id: root
    objectName: "markdownTable"
    property var tableData: ({})
    property var headers: tableData.headers || []
    property var rows: tableData.rows || []
    property var alignment: tableData.alignment || []
    property int cellWidth: 190
    signal copyRequested(string content)
    signal externalLinkRequested(string url)

    implicitHeight: card.implicitHeight

    Rectangle {
        id: card
        width: root.width
        implicitHeight: container.implicitHeight + 16
        radius: 8
        color: "#24272b"
        border.width: 1
        border.color: "#4e4e4e"

        Column {
            id: container
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 8
            spacing: 5

            RowLayout {
                width: parent.width
                height: 27

                Label {
                    text: "Tabla"
                    font.pixelSize: 12
                    color: "#afafaf"
                    Layout.fillWidth: true
                }
                CopyFeedbackButton {
                    stableWidth: 94
                    contentToCopy: root.tableData.raw || ""
                    onCopyRequested: function(value) {
                        root.copyRequested(value)
                    }
                }
            }

            ScrollView {
                id: tableScroll
                width: parent.width
                height: Math.min(350, body.implicitHeight + 4)
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AsNeeded
                ScrollBar.vertical.policy: ScrollBar.AsNeeded

                Column {
                    id: body
                    width: root.headers.length * root.cellWidth
                    spacing: 0

                    Repeater {
                        model: 1 + root.rows.length
                        Row {
                            id: tableRow
                            required property int index
                            property bool isHeader: index === 0
                            property var cells: isHeader ? root.headers : root.rows[index - 1]
                            width: body.width
                            height: implicitHeight
                            spacing: 0

                            Repeater {
                                model: root.headers.length
                                Rectangle {
                                    required property int index
                                    property var cell: tableRow.cells[index]
                                    width: root.cellWidth
                                    implicitHeight: Math.max(35, cellText.implicitHeight + 14)
                                    height: tableRow.height
                                    color: tableRow.isHeader ? "#323c45"
                                        : tableRow.index % 2 ? "#282d32" : "#2e343a"
                                    border.width: 1
                                    border.color: "#4e4e4e"

                                    ProseText {
                                        id: cellText
                                        anchors.left: parent.left
                                        anchors.top: parent.top
                                        anchors.margins: 7
                                        width: Math.max(0, parent.width - 14)
                                        formatted: parent.cell ? parent.cell.formatted : ""
                                        styled: true
                                        font.bold: tableRow.isHeader
                                        color: "#f6f6f6"
                                        horizontalAlignment: root.alignment[index] === "right"
                                            ? TextEdit.AlignRight
                                            : root.alignment[index] === "center"
                                              ? TextEdit.AlignHCenter : TextEdit.AlignLeft
                                        onExternalLinkRequested: function(url) {
                                            root.externalLinkRequested(url)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
