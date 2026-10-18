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

    // добавляем отступ между карточками, ListItemType сам не разделяет
    // элементы в Repeater по вертикали
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

                title: qsTr("Генератор WARP")
                description: qsTr("Бесплатные конфигурации Cloudflare WARP для AmneziaWG")
                backButtonVisible: root.stackView !== null && root.stackView.depth > 1
            }

            BasicCardType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Как это работает")
                subtitle: qsTr("Каждый конфиг получает свою пару ключей прямо на этом компьютере. "
                             + "Публичный ключ уходит в Cloudflare, приватный остаётся в конфиге. "
                             + "Адреса и порты случайные, чтобы трафик не выделялся на фоне остального.")
            }

            ButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: WarpController.hasConfigs ? qsTr("Сгенерировать заново")
                                                : qsTr("Сгенерировать 5 конфигов")

                enabled: !WarpController.busy

                onClicked: WarpController.generate(5)
            }

            // ---- прогресс и статус ----
            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: WarpController.busy || WarpController.statusText !== ""
                spacing: 12

                ProgressBarType {
                    Layout.fillWidth: true

                    visible: WarpController.busy
                    indeterminate: WarpController.busy
                }

                SmallTextType {
                    Layout.fillWidth: true

                    text: WarpController.statusText
                    wrapMode: Text.WordWrap
                }
            }

            // ---- список серверов ----
            Repeater {
                model: WarpController.configs

                delegate: ListItemType {
                    required property int index
                    required property var modelData

                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16

                    visible: !WarpController.busy
                    enabled: !WarpController.busy

                    text: modelData.name
                    subtitle: modelData.endpoint

                    trailingIconSource: "qrc:/images/controls/chevron-right.svg"
                    showSeparator: false

                    onClicked: {
                        // текст конфига уходит прямо в импорт,
                        // без диалога выбора файла
                        if (ImportController.extractConfigFromData(WarpController.configText(index))) {
                            PageController.goToPage(PageEnum.PageSetupWizardViewConfig)
                        }
                    }
                }
            }

            // ---- папка с конфигами ----
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

                    text: qsTr("Открыть папку с конфигами")

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
