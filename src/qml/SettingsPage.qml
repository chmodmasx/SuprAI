import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: page

    required property var settings
    required property color panelColor
    required property color panelAltColor
    required property color borderColor
    required property color foregroundColor
    required property color mutedColor
    required property color accentColor

    color: "#1f1f1f"

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: content.implicitHeight + 48
        clip: true

        ColumnLayout {
            id: content

            width: Math.min(parent.width - 64, 760)
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 32
            spacing: 18

            Label {
                text: "Configuración"
                color: page.foregroundColor
                font.pixelSize: 26
                font.bold: true
            }

            Label {
                Layout.fillWidth: true
                text: "Estado actual del runtime y del provider. La edición persistente se habilitará sólo donde exista un backend seguro para secretos."
                color: page.mutedColor
                wrapMode: Text.Wrap
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: providerColumn.implicitHeight + 32
                radius: 12
                color: page.panelColor
                border.color: page.borderColor

                ColumnLayout {
                    id: providerColumn
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    Label {
                        text: "Runtime y provider"
                        color: page.foregroundColor
                        font.pixelSize: 17
                        font.bold: true
                    }

                    Label {
                        text: "Runtime"
                        color: page.mutedColor
                        font.pixelSize: 11
                    }

                    TextField {
                        Layout.fillWidth: true
                        text: page.settings.runtimeMode
                        readOnly: true
                    }

                    Label {
                        text: "Modelo"
                        color: page.mutedColor
                        font.pixelSize: 11
                    }

                    TextField {
                        Layout.fillWidth: true
                        text: page.settings.model
                        readOnly: true
                    }

                    Label {
                        text: "Endpoint"
                        color: page.mutedColor
                        font.pixelSize: 11
                    }

                    TextField {
                        Layout.fillWidth: true
                        text: page.settings.baseUrl
                        readOnly: true
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: secretColumn.implicitHeight + 32
                radius: 12
                color: page.panelAltColor
                border.color: page.borderColor

                ColumnLayout {
                    id: secretColumn
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 8

                    Label {
                        text: "Credenciales"
                        color: page.foregroundColor
                        font.pixelSize: 17
                        font.bold: true
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "SUPRAI_API_KEY se lee del entorno y no se persiste en QSettings. No habrá fallback silencioso a texto plano."
                        color: page.mutedColor
                        wrapMode: Text.Wrap
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                text: "Variables actuales: SUPRAI_RUNTIME, SUPRAI_BASE_URL, SUPRAI_MODEL, SUPRAI_API_KEY y SUPRAI_SYSTEM_PROMPT."
                color: page.mutedColor
                wrapMode: Text.Wrap
                font.pixelSize: 12
            }
        }
    }
}
