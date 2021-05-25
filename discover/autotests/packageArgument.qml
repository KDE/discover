import QtQuick 2.0
import QtTest 1.1

DiscoverTest
{
    function test_open() {
        verify(appRoot.stack.currentItem, "has a page");
        while (appRoot.stack.currentItem.title === "Loading…")
            waitForRendering();
        compare(appRoot.stack.currentItem.application.packageName, "CMakeLists.txt", "pkgname");
    }
}
