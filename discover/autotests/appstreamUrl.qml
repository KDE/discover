import QtQuick 2.0
import QtTest 1.1

DiscoverTest
{
    function test_open() {
        verify(appRoot.stack.currentItem, "has a page");
        while (appRoot.stack.currentItem.title === "")
            verify(waitForRendering());
        compare(appRoot.currentTopLevel, appRoot.topBrowsingComp)
        compare(appRoot.stack.currentItem.title, "techie1", "same title");
    }
}
