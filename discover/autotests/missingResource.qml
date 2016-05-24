import QtQuick 2.0
import QtTest 1.1

DiscoverTest
{
    function test_open() {
        verify(!appRoot.defaultStartup)
        compare(typeName(appRoot.stack.currentItem), "BrowsingPage")
    }
}
