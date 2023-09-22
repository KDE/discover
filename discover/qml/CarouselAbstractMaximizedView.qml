/*
 *   SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQml
import QtQuick
import org.kde.discover

QtObject {
    required property int currentIndex
    required property ScreenshotsModel model
    required property Window transientParent
    required property int mode

    function close(animated: bool) {}

    property bool toggleModeAvailable
    signal toggleMode()

    // Popup and Window differ in how they set background graphics
    readonly property color backgroundColor: "#262828"
}
