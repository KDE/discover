import QtQuick 2.0
import org.kde.discover.app 1.0
import QtTest 1.1

DiscoverTest
{
    function test_openResource() {
        app.openApplication("dummy://dummy.1");
        verify(waitForSignal(appRoot.stack, "currentItemChanged"))
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
        verify(waitForSignal(appRoot.stack, "currentItemChanged"))
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
