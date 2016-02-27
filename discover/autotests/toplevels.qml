import QtQuick 2.0
import org.kde.discover.app 1.0

DiscoverTest
{
    onReset: {
        appRoot.currentTopLevel = appRoot.topBrowsingComp
    }

    function test_openResource() {
        var resourceName = "Dummy 1";
        app.openApplication(resourceName);
        verify(appRoot.stack.currentItem, "has a page");
        compare(appRoot.stack.currentItem.title, "Dummy 1", "same title");

        var button = findChild(appRoot.stack.currentItem, "InstallApplicationButton")
        verify(!button.isActive)
        button.click()
        verify(button.isActive)
        waitForSignal(button, "isActiveChanged")
        verify(!button.isActive)
    }

    function test_openCategory() {
        var categoryName = "dummy";
        app.openCategory(categoryName);
        verify(appRoot.stack.currentItem, "has a page");
        compare(appRoot.stack.currentItem.title, "dummy", "same title");
    }

    function test_modes() {
        app.openMode("Browsing");
        compare(appRoot.currentTopLevel, appRoot.topBrowsingComp, "correct component, browsing");

        app.openMode("Installed");
        compare(appRoot.currentTopLevel, appRoot.topInstalledComp, "correct component, installed");

        app.openMode("Update");
        compare(appRoot.currentTopLevel, appRoot.topUpdateComp, "correct component, updates");

        app.openMode("Sources");
        compare(appRoot.currentTopLevel, appRoot.topSourcesComp, "correct component, sources");
    }
}
