pragma Singleton

import QtQml 2.0
import QtQuick.Window 2.2
import org.kde.discover.app 1.0

QtObject
{
    readonly property QtObject mainWindow: app
    property int compactMode: app.compactMode

    readonly property real pixelDensity: app.Screen.pixelDensity

    ///we'll use compact if the width of the window is less than 10cm
    readonly property bool isCompact: compactMode!=MuonDiscoverMainWindow.Auto ? compactMode==MuonDiscoverMainWindow.Compact : (app.width/pixelDensity<100)
    readonly property real actualWidth: isCompact ? app.width : app.width-Math.pow(app.width/70., 2);
}
