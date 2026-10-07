import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root

    width: 1320
    height: 820
    minimumWidth: 980
    minimumHeight: 620
    visible: true
    title: "SuprAI"

    color: "#1f1f1f"

    readonly property color pageColor: "#1f1f1f"
    readonly property color panel: "#292929"
    readonly property color panelAlt: "#313131"
    readonly property color border: "#4E4E4E"
    readonly property color accent: "#51A2DA"
    readonly property color foreground: "#F6F6F6"
    readonly property color muted: "#AFAFAF"

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.preferredWidth: 230
            Layout.fillHeight: true
            color: root.panel
            border.color: root.border
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 12

                Label {
                    text: "SuprAI"
                    color: root.foreground
                    font.pixelSize: 23
                    font.bold: true
                }

                Label {
                    text: "Native prototype"
                    color: root.muted
                    font.pixelSize: 12
                }

                Button {
                    Layout.fillWidth: true
                    text: "+ Nueva conversación"
                    enabled: !chatController.busy
                    onClicked: chatController.newConversation()
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: root.border
                }

                Label {
                    text: "Sesiones"
                    color: root.muted
                    font.pixelSize: 12
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 46
                    radius: 8
                    color: root.panelAlt

                    Label {
                        anchors.fill: parent
                        anchors.margins: 12
                        verticalAlignment: Text.AlignVCenter
                        text: "Sesión actual"
                        color: root.foreground
                        elide: Text.ElideRight
                    }
                }

                Item {
                    Layout.fillHeight: true
                }

                Label {
                    Layout.fillWidth: true
                    text: appSettings.runtimeMode === "mock" ? "MockRuntime" : "NativeSuprAIRuntime"
                    color: root.muted
                    wrapMode: Text.Wrap
                    font.pixelSize: 11
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: root.pageColor

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 64
                    color: root.panel
                    border.color: root.border
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 20
                        anchors.rightMargin: 20

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Label {
                                text: appSettings.model
                                color: root.foreground
                                font.pixelSize: 15
                                font.bold: true
                            }

                            Label {
                                text: appSettings.baseUrl
                                color: root.muted
                                font.pixelSize: 11
                                elide: Text.ElideMiddle
                                Layout.fillWidth: true
                            }
                        }

                        Rectangle {
                            implicitWidth: statusLabel.implicitWidth + 20
                            implicitHeight: 30
                            radius: 15
                            color: chatController.busy ? "#3a4c59" : "#314238"

                            Label {
                                id: statusLabel
                                anchors.centerIn: parent
                                text: chatController.reasoning
                                      ? "Reasoning…"
                                      : chatController.runtimeState
                                color: root.foreground
                                font.pixelSize: 12
                            }
                        }
                    }
                }

                Rectangle {
                    visible: chatController.lastError.length > 0
                    Layout.fillWidth: true
                    implicitHeight: errorText.implicitHeight + 24
                    color: "#4a2929"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 12

                        Label {
                            id: errorText
                            Layout.fillWidth: true
                            text: chatController.lastError
                            color: root.foreground
                            wrapMode: Text.Wrap
                        }

                        ToolButton {
                            text: "×"
                            onClicked: chatController.clearError()
                        }
                    }
                }

                ListView {
                    id: transcript

                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    clip: true
                    spacing: 12
                    leftMargin: 22
                    rightMargin: 22
                    topMargin: 22
                    bottomMargin: 22
                    model: chatController.transcript
                    reuseItems: true

                    ScrollBar.vertical: ScrollBar {}

                    onCountChanged: Qt.callLater(function() {
                        if (count > 0)
                            positionViewAtEnd()
                    })

                    delegate: Item {
                        required property string itemId
                        required property string speaker
                        required property string text
                        required property bool streaming

                        width: ListView.view.width - transcript.leftMargin - transcript.rightMargin
                        implicitHeight: bubble.implicitHeight

                        Rectangle {
                            id: bubble

                            width: Math.min(parent.width * 0.84, 780)
                            implicitHeight: body.implicitHeight + 26
                            radius: 12

                            anchors.right: speaker === "user" ? parent.right : undefined
                            anchors.left: speaker === "user" ? undefined : parent.left

                            color: speaker === "user" ? "#24445A" : root.panelAlt
                            border.color: speaker === "user" ? root.accent : root.border
                            border.width: 1

                            Text {
                                id: body

                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: 13

                                text: parent.parent.text + (parent.parent.streaming ? " ▋" : "")
                                color: root.foreground
                                wrapMode: Text.Wrap
                                textFormat: Text.PlainText
                                font.pixelSize: 14
                                lineHeight: 1.25
                            }
                        }
                    }

                    Label {
                        anchors.centerIn: parent
                        visible: transcript.count === 0
                        text: "Escribí un mensaje para iniciar el primer turno."
                        color: root.muted
                        font.pixelSize: 14
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.max(116, composer.implicitHeight + 34)
                    color: root.panel
                    border.color: root.border
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 10

                        TextArea {
                            id: composer

                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            placeholderText: "Mensaje a SuprAI…"
                            color: root.foreground
                            placeholderTextColor: root.muted
                            wrapMode: TextEdit.Wrap
                            selectByMouse: true
                            background: Rectangle {
                                radius: 10
                                color: root.panelAlt
                                border.color: composer.activeFocus ? root.accent : root.border
                            }

                            Keys.onPressed: function(event) {
                                if (event.key === Qt.Key_Return
                                        && !(event.modifiers & Qt.ShiftModifier)
                                        && !chatController.busy) {
                                    var message = composer.text.trim()
                                    if (message.length > 0) {
                                        chatController.sendMessage(message)
                                        composer.clear()
                                    }
                                    event.accepted = true
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.preferredWidth: 92

                            Button {
                                Layout.fillWidth: true
                                text: "Enviar"
                                enabled: !chatController.busy && composer.text.trim().length > 0
                                onClicked: {
                                    var message = composer.text.trim()
                                    if (message.length > 0) {
                                        chatController.sendMessage(message)
                                        composer.clear()
                                    }
                                }
                            }

                            Button {
                                Layout.fillWidth: true
                                text: "Detener"
                                enabled: chatController.busy
                                onClicked: chatController.cancel()
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.preferredWidth: 280
            Layout.fillHeight: true
            color: root.panel
            border.color: root.border
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                Label {
                    text: "Inspector"
                    color: root.foreground
                    font.pixelSize: 18
                    font.bold: true
                }

                Label {
                    text: "Runtime"
                    color: root.muted
                    font.pixelSize: 11
                }

                Label {
                    Layout.fillWidth: true
                    text: appSettings.runtimeMode
                    color: root.foreground
                    wrapMode: Text.Wrap
                }

                Label {
                    text: "Estado"
                    color: root.muted
                    font.pixelSize: 11
                }

                Label {
                    text: chatController.runtimeState
                    color: root.foreground
                }

                Label {
                    text: "Modelo"
                    color: root.muted
                    font.pixelSize: 11
                }

                Label {
                    Layout.fillWidth: true
                    text: appSettings.model
                    color: root.foreground
                    wrapMode: Text.Wrap
                }

                Label {
                    text: "Endpoint"
                    color: root.muted
                    font.pixelSize: 11
                }

                Label {
                    Layout.fillWidth: true
                    text: appSettings.baseUrl
                    color: root.foreground
                    wrapMode: Text.WrapAnywhere
                    font.pixelSize: 12
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: root.border
                }

                Label {
                    Layout.fillWidth: true
                    text: "El reasoning separado del provider se trata como estado efímero: este prototipo no lo agrega al historial que se reenvía en turnos posteriores."
                    color: root.muted
                    wrapMode: Text.Wrap
                    font.pixelSize: 12
                }

                Item {
                    Layout.fillHeight: true
                }

                Label {
                    Layout.fillWidth: true
                    text: "Configuración por entorno:\nSUPRAI_RUNTIME\nSUPRAI_BASE_URL\nSUPRAI_MODEL\nSUPRAI_API_KEY"
                    color: root.muted
                    wrapMode: Text.Wrap
                    font.pixelSize: 11
                }
            }
        }
    }
}
