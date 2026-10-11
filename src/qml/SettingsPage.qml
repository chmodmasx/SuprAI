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
        contentHeight: form.implicitHeight + 64
        clip: true

        ColumnLayout {
            id: form
            width: Math.min(parent.width - 64, 760)
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 32
            spacing: 16

            Label {
                text: "Configuración de modelo"
                color: page.foregroundColor
                font.pixelSize: 25
                font.bold: true
            }

            Label {
                Layout.fillWidth: true
                text: "Servidor OpenAI-compatible activo. Los cambios se guardan para el próximo inicio de SuprAI; la conversación actual no cambia de modelo a mitad de turno."
                color: page.mutedColor
                wrapMode: Text.Wrap
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: fields.implicitHeight + 32
                radius: 12
                color: page.panelColor
                border.color: page.borderColor

                ColumnLayout {
                    id: fields
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 11

                    Label {
                        text: "Endpoint (/v1)"
                        color: page.foregroundColor
                        font.bold: true
                    }
                    TextField {
                        id: endpointField
                        Layout.fillWidth: true
                        text: page.settings.baseUrl
                        placeholderText: "http://127.0.0.1:8090/v1"
                        selectByMouse: true
                    }

                    Label {
                        text: "API key (KWallet / Secret Service)"
                        color: page.foregroundColor
                        font.bold: true
                    }
                    TextField {
                        id: keyField
                        Layout.fillWidth: true
                        echoMode: TextInput.Password
                        placeholderText: page.settings.hasStoredKey
                            ? "Clave segura guardada. Dejá vacío para conservarla."
                            : "Introducí una clave si el servidor la exige."
                        selectByMouse: true
                    }
                    Label {
                        Layout.fillWidth: true
                        text: "La clave no se guarda en QSettings ni en la base de conversaciones. Para servidores públicos se exige HTTPS."
                        font.pixelSize: 12
                        color: page.mutedColor
                        wrapMode: Text.Wrap
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Button {
                            text: "Buscar modelos"
                            enabled: !page.settings.busy
                            onClicked: page.settings.discoverModels(endpointField.text, keyField.text)
                        }
                        BusyIndicator {
                            running: page.settings.busy
                            visible: running
                            implicitHeight: 26
                            implicitWidth: 26
                        }
                        Item { Layout.fillWidth: true }
                        Button {
                            text: "Borrar clave guardada"
                            enabled: page.settings.hasStoredKey && !page.settings.busy
                            onClicked: page.settings.deleteStoredKey()
                        }
                    }

                    Label {
                        text: "Modelo"
                        color: page.foregroundColor
                        font.bold: true
                    }
                    ComboBox {
                        id: modelsBox
                        Layout.fillWidth: true
                        model: page.settings.discoveredModels
                        visible: count > 0
                        onActivated: modelField.text = currentText
                    }
                    TextField {
                        id: modelField
                        Layout.fillWidth: true
                        text: page.settings.model
                        placeholderText: "Elegí un modelo o ingresá su ID"
                        selectByMouse: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Item { Layout.fillWidth: true }
                        Button {
                            text: "Guardar configuración"
                            enabled: !page.settings.busy
                            onClicked: {
                                page.settings.saveProvider(endpointField.text, modelField.text, keyField.text)
                                keyField.clear()
                            }
                        }
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                visible: page.settings.restartRequired
                text: "Cambios guardados: para activar el modelo, cerrá SuprAI completamente y volvé a abrir el AppImage."
                color: page.accentColor
                font.bold: true
                wrapMode: Text.Wrap
            }

            Label {
                Layout.fillWidth: true
                visible: page.settings.statusMessage.length > 0
                text: page.settings.statusMessage
                color: page.foregroundColor
                wrapMode: Text.Wrap
            }

            Label {
                Layout.fillWidth: true
                text: "Servidor actualmente activo: " + (page.settings.activeModel || "(sin modelo)")
                    + " · " + (page.settings.activeBaseUrl || "(sin endpoint)")
                color: page.mutedColor
                font.pixelSize: 12
                wrapMode: Text.WrapAnywhere
            }

            Label {
                Layout.fillWidth: true
                text: "Las variables SUPRAI_BASE_URL, SUPRAI_MODEL y SUPRAI_API_KEY, si están definidas al iniciar, tienen prioridad sobre las preferencias guardadas."
                color: page.mutedColor
                font.pixelSize: 12
                wrapMode: Text.Wrap
            }
        }
    }
}
