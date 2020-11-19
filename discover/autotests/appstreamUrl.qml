import QtQuick 2.0
import QtTest 1.1

DiscoverTest
{
    function test_open() {
        verify(appRoot.stack.currentItem, "has a loading page");
        compare(appRoot.stack.currentItem.title, "techie1", "same title");
        compare(appRoot.stack.currentItem.application.packageName, "techie1", "pkgname");
    }
}
