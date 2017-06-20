import QtQuick 2.0
import org.kde.discover.app 1.0
import QtTest 1.1

DiscoverTest
{
    function test_openResource() {
        app.openApplication("dummy://dummy.1");
        compare(appRoot.stack.currentItem.title, "dummy://dummy.1", "same title");
        compare(appRoot.stack.currentItem.isBusy, true, "same title");
        verify(waitForSignal(appRoot.stack.currentItem, "isBusyChanged"))

        appRoot.stack.currentItem.flickable.currentIndex = 0
        var item = appRoot.stack.currentItem.flickable.currentItem
        verify(item)
        item.clicked()
        verify(appRoot.stack.currentItem, "has a page");
        compare(appRoot.stack.currentItem.title, "Dummy 1", "same title");

        var button = findChild(appRoot.stack.currentItem, "InstallApplicationButton")
        verify(!button.isActive)
        button.click()
        verify(button.isActive)
        verify(waitForSignal(button, "isActiveChanged"))
        verify(!button.isActive)
    }

    SignalSpy {
        id: cancelSpy
        target: TransactionModel
        signalName: "transactionRemoved"
    }
    function test_cancel() {
        app.openApplication("dummy://dummy.2");
        compare(appRoot.stack.currentItem.title, "dummy://dummy.2", "same title");
        compare(appRoot.stack.currentItem.isBusy, true, "same title");
        verify(waitForSignal(appRoot.stack.currentItem, "isBusyChanged"))

        appRoot.stack.currentItem.flickable.currentIndex = 0
        var item = appRoot.stack.currentItem.flickable.currentItem
        verify(item)
        item.clicked()
        verify(appRoot.stack.currentItem, "has a page");
        compare(appRoot.stack.currentItem.title, "Dummy 2", "same title");

        var button = findChild(appRoot.stack.currentItem, "InstallApplicationButton")
        verify(!button.isActive)
        
        cancelSpy.clear()
        var state = button.application.state;
        
        button.click()
        verify(button.isActive)
        button.listener.cancel()
        verify(!button.isActive)
        compare(cancelSpy.count, 1)
        verify(state == button.application.state)
    }
}
