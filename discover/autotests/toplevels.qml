import QtQuick 2.0
import org.kde.discover.app 1.0
import QtTest 1.1

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
        verify(waitForRendering())
    }

    function test_update() {
        app.openMode("Update");

        var updatePage = appRoot.stack.currentItem
        var button = findChild(updatePage, "Button")
        verify(button);
        button.clicked();
        verify(updatePage.condition)

        //make sure the window doesn't close while updating
        verify(app.visible);
        verify(waitForRendering())
        app.close();
        verify(app.visible);

        waitForSignal(updatePage, "conditionChanged")
        verify(!updatePage.condition)
    }

    function test_modes() {
        app.openMode("Browsing");
        compare(appRoot.currentTopLevel, appRoot.topBrowsingComp, "correct component, browsing");
        verify(waitForRendering())

        app.openMode("Installed");
        compare(appRoot.currentTopLevel, appRoot.topInstalledComp, "correct component, installed");
        verify(waitForRendering())

        app.openMode("Update");
        compare(appRoot.currentTopLevel, appRoot.topUpdateComp, "correct component, updates");
        verify(waitForRendering())

        app.openMode("Sources");
        compare(appRoot.currentTopLevel, appRoot.topSourcesComp, "correct component, sources");
        verify(waitForRendering())
    }
}
