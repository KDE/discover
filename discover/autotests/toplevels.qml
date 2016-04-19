import QtQuick 2.0
import org.kde.discover.app 1.0
import QtTest 1.1

DiscoverTest
{
    property QtObject appRoot
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
        verify(waitForSignal(button, "isActiveChanged"))
        verify(!button.isActive)
    }

    function test_openCategory() {
        var categoryName = "dummy 3";
        app.openCategory(categoryName);
        verify(appRoot.stack.currentItem, "has a page");
        compare(appRoot.stack.currentItem.title, categoryName, "same title");
        verify(waitForRendering())

        categoryName = "dummy 4";
        app.openCategory(categoryName);
        verify(appRoot.stack.currentItem, "has a page");
        compare(appRoot.stack.currentItem.title, categoryName, "same title");
        verify(waitForRendering())
    }

    function test_update() {
        app.openMode("Update");

        var updatePage = appRoot.stack.currentItem;
        verify(isType(updatePage, "UpdatesPage"), updatePage.toString());
        compare(updatePage.condition, true, "to update")
        var button = findChild(updatePage, "Button")
        verify(button);
        button.clicked();
        compare(updatePage.condition, true, "updating")

        //make sure the window doesn't close while updating
        verify(appRoot.visible);
        verify(waitForRendering())
        appRoot.close()
        verify(appRoot.visible);

        while(ResourcesModel.updatesCount>0)
            verify(waitForSignal(ResourcesModel, "updatesCountChanged"))
        compare(updatePage.condition, false, "update finished")
        compare(ResourcesModel.updatesCount, 0, "should be up to date")
    }

    function test_search() {
        app.openMode("Browsing");
        var searchField = findChild(appRoot, "TextField");
        verify(searchField);
        searchField.text = "cocacola"
        verify(waitForSignal(appRoot.stack, "currentItemChanged"))
        while(!isType(appRoot.stack.currentItem, "ApplicationsListPage"))
            verify(waitForSignal(appRoot.stack, "currentItemChanged"))
        searchField.text = "dummy"
        var listPage = appRoot.stack.currentItem
        compare(listPage.state, "list")
        verify(waitForSignal(listPage.model, "countChanged"))
        compare(listPage.model.count, ResourcesModel.rowCount()/2)
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
