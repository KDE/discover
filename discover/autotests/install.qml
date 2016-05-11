import QtQuick 2.0
import org.kde.discover.app 1.0
import QtTest 1.1

DiscoverTest
{
    function test_openResource() {
        var resourceName = "Dummy 1";
        app.openApplication(resourceName);
        verify(appRoot.stack.currentItem, "has a page");
        compare(appRoot.stack.currentItem.title, resourceName, "same title");

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
        signalName: "transactionCancelled"
    }
    function test_cancel() {
        var resourceName = "Dummy 2";
        app.openApplication(resourceName);
        verify(appRoot.stack.currentItem, "has a page");
        compare(appRoot.stack.currentItem.title, resourceName, "same title");

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
