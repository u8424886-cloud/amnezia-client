import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Config"
import "../Controls2/TextTypes"
import "../Components"

PageType {
    id: root

    Connections {
        target: WarpController

        function onBusyChanged() {
            PageController.showBusyIndicator(WarpController.busy)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        FlickableType {
            id: flickable

            anchors.top: parent.top
            anchors.bottom: parent.bottom

            contentHeight: content.implicitHeight

            ColumnLayout {
                id: content

                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right

                spacing: 0

                NavigationBarType {
                    Layout.fillWidth: true

                    title: qsTr("WARP Generator")
                    description: qsTr("Free Cloudflare WARP configs for AmneziaWG")
                    backButtonVisible: root.stackView !== null && root.stackView.depth > 1
                }

                BasicCardType {
                    Layout.fillWidth: true
                    Layout.topMargin: 16
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16

                    text: qsTr("How it works")
                    subtitle: qsTr("Every config gets its own key pair right on this computer. The public key "
                                 + "goes to Cloudflare, the private key stays in the config. Addresses and ports "
                                 + "are random so the traffic does not stand out.")
                }

                ButtonType {
                    Layout.fillWidth: true
                    Layout.topMargin: 16
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16

                    text: WarpController.hasConfigs ? qsTr("Regenerate")
                                                   : qsTr("Generate 5 configs")

                    enabled: !WarpController.busy

                    onClicked: WarpController.generate(5)
                }

                SmallTextType {
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16

                    visible: WarpController.statusText !== ""

                    text: WarpController.statusText
                    wrapMode: Text.WordWrap
                }

                Repeater {
                    model: WarpController.configs

                    delegate: ListItemType {
                        required property int index
                        required property var modelData

                        Layout.fillWidth: true
                        Layout.topMargin: 8
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16

                        visible: !WarpController.busy
                        enabled: !WarpController.busy

                        text: modelData.name
                        subtitle: modelData.endpoint

                        trailingIconSource: "qrc:/images/controls/chevron-right.svg"
                        showSeparator: false

                        onClicked: {
                            // The config text goes straight into the import,
                            // no file selection dialog.
                            if (ImportController.extractConfigFromData(WarpController.configText(index))) {
                                PageController.goToPage(PageEnum.PageSetupWizardViewConfig)
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 20
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 24 + PageController.safeAreaBottomMargin

                    visible: WarpController.hasConfigs

                    spacing: 12

                    ButtonType {
                        Layout.fillWidth: true

                        text: qsTr("Open configs folder")

                        onClicked: WarpController.openConfigsFolder()
                    }

                    SmallTextType {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignHCenter

                        text: WarpController.configsPath
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
        }
    }
}
