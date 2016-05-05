import QtQuick 2.0
import QtTest 1.1

DiscoverTest
{
    property QtObject appRoot

    function test_open() {
        verify(appRoot.stack.currentItem, "has a page");
        while (appRoot.stack.currentItem.title === "")
            verify(waitForRendering());
        compare(appRoot.stack.currentItem.title, "techie1", "same title");
    }
}
