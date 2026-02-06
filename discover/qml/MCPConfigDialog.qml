/*
 *   SPDX-FileCopyrightText: 2025 Yakup Atahanov
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.discover as Discover

Kirigami.OverlaySheet {
    id: sheet

    property Discover.Transaction transaction
    property Discover.AbstractResource resource

    property bool acted: false
    property bool isReconfiguration: false
    property var propertyValues: ({})

    title: i18n("Configure %1", resource ? resource.name : "")

    showCloseButton: false

    implicitWidth: Kirigami.Units.gridUnit * 30

    ColumnLayout {
        Layout.fillWidth: true
        spacing: Kirigami.Units.largeSpacing

        QQC2.Label {
            Layout.fillWidth: true
            Layout.maximumWidth: Kirigami.Units.gridUnit * 20

            text: i18n("Please provide the required configuration properties for this MCP server.")
            wrapMode: Text.WordWrap
            textFormat: Text.PlainText
        }

        Kirigami.FormLayout {
            Layout.fillWidth: true

            Repeater {
                id: propertiesRepeater
                model: resource ? resource.requiredProperties : []

                delegate: ColumnLayout {
                    required property var modelData

                    Kirigami.FormData.label: modelData.label + (modelData.required ? " *" : "")
                    Layout.fillWidth: true

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.smallSpacing

                        QQC2.TextField {
                            id: field

                            Layout.fillWidth: true

                            readonly property string propertyKey: modelData.key
                            readonly property bool isSensitive: modelData.sensitive

                            echoMode: isSensitive && !showPasswordCheckBox.checked
                                ? QQC2.TextField.Password
                                : QQC2.TextField.Normal

                            text: {
                                if (!resource || !resource.propertyValues) {
                                    return ""
                                }
                                const values = resource.propertyValues
                                return values[propertyKey] || ""
                            }

                            onTextChanged: {
                                const newValues = {}
                                // Copy existing values
                                if (resource && resource.propertyValues) {
                                    const existingValues = resource.propertyValues
                                    for (const key in existingValues) {
                                        newValues[key] = existingValues[key]
                                    }
                                }
                                // Copy current propertyValues
                                for (const key in propertyValues) {
                                    newValues[key] = propertyValues[key]
                                }
                                // Update with current field value
                                if (text.length > 0) {
                                    newValues[propertyKey] = text
                                } else {
                                    delete newValues[propertyKey]
                                }
                                propertyValues = newValues
                            }

                            placeholderText: modelData.description || ""
                        }

                        QQC2.CheckBox {
                            id: showPasswordCheckBox

                            visible: modelData.sensitive
                            icon.name: checked ? "visibility" : "hint"
                            icon.width: Kirigami.Units.iconSizes.smallMedium
                            icon.height: Kirigami.Units.iconSizes.smallMedium

                            QQC2.ToolTip.visible: hovered
                            QQC2.ToolTip.text: checked ? i18n("Hide password") : i18n("Show password")
                        }
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        visible: modelData.description && modelData.description.length > 0

                        text: modelData.description
                        wrapMode: Text.WordWrap
                        font: Kirigami.Theme.smallFont
                        opacity: 0.7
                    }
                }
            }
        }

        QQC2.Label {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.smallSpacing

            text: {
                // Validate required fields
                if (!resource) {
                    return ""
                }
                const props = resource.requiredProperties || []
                for (let i = 0; i < props.length; i++) {
                    const prop = props[i]
                    if (prop.required) {
                        const value = propertyValues[prop.key] || ""
                        if (value.length === 0) {
                            return i18n("Please fill in all required fields (marked with *)")
                        }
                    }
                }
                return ""
            }

            wrapMode: Text.WordWrap
            color: Kirigami.Theme.negativeTextColor
            visible: text.length > 0
        }
    }

    footer: RowLayout {
        Item { Layout.fillWidth: true }

        QQC2.Button {
            text: i18n("Cancel")
            icon.name: "dialog-cancel"
            onClicked: {
                if (transaction) {
                    transaction.cancel()
                }
                sheet.acted = true
                sheet.close()
            }
            Keys.onEscapePressed: clicked()
        }

        QQC2.Button {
            text: i18n("Continue")
            icon.name: "dialog-ok"
            enabled: {
                // Validate required fields
                if (!resource) {
                    return false
                }
                const props = resource.requiredProperties || []
                for (let i = 0; i < props.length; i++) {
                    const prop = props[i]
                    if (prop.required) {
                        const value = propertyValues[prop.key] || ""
                        if (value.length === 0) {
                            return false
                        }
                    }
                }
                return true
            }
            onClicked: {
                // Convert propertyValues object to QMap
                const configMap = {}
                for (const key in propertyValues) {
                    configMap[key] = propertyValues[key]
                }
                
                if (isReconfiguration) {
                    // Update configuration directly on resource
                    resource.updateConfiguration(configMap);
                } else {
                    // During installation, use transaction
                    transaction.setConfiguration(configMap);
                }
                
                sheet.acted = true
                sheet.close()
            }
            Keys.onEnterPressed: clicked()
            Keys.onReturnPressed: clicked()
        }
    }

    onVisibleChanged: if (!visible) {
        sheet.destroy(1000)
        if (!sheet.acted && transaction) {
            transaction.cancel()
        }
    }

    Component.onCompleted: {
        // Initialize propertyValues from resource
        if (resource && resource.propertyValues) {
            const values = {}
            const resourceValues = resource.propertyValues
            for (const key in resourceValues) {
                values[key] = resourceValues[key]
            }
            propertyValues = values
        }
    }
}
