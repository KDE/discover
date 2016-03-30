pragma Singleton

import QtQml 2.0
import QtQuick.Window 2.2
import org.kde.discover.app 1.0

QtObject
{
    id: root
    property QtObject mainWindow: null
    property int compactMode: app.compactMode

    ///we'll use compact if the width of the window is less than 10cm
    readonly property bool isCompact: (!root.mainWindow || compactMode!=MuonDiscoverMainWindow.Auto) ? compactMode==MuonDiscoverMainWindow.Compact : (root.mainWindow.width/root.mainWindow.Screen.pixelDensity<100)
    readonly property real actualWidth: isCompact ? root.mainWindow.width : root.mainWindow.width-Math.pow(root.mainWindow.width/70., 2)
}
