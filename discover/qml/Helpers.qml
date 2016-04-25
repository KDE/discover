pragma Singleton

import QtQml 2.0
import QtQuick.Window 2.2
import org.kde.discover.app 1.0

QtObject
{
    id: root
    property QtObject mainWindow: null
    property int compactMode: app.compactMode

    readonly property real width: root.mainWindow ? root.mainWindow.width : 0
    ///we'll use compact if the width of the window is less than 10cm
    readonly property bool isCompact: (!root.mainWindow || compactMode!=MuonDiscoverMainWindow.Auto) ? compactMode==MuonDiscoverMainWindow.Compact : (width/root.mainWindow.Screen.pixelDensity<100)
}
