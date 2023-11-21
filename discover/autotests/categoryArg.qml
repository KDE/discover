import QtQuick
import QtTest

DiscoverTest {
    function test_openCategory() {
        verify(appRoot.stack.currentItem, "has a page");
        while (appRoot.stack.currentItem.title === "")
            verify(waitForRendering());
        compare(appRoot.stack.currentItem.title, "dummy 2.1", "same title");
    }
}
