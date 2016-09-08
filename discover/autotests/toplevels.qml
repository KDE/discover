import QtQuick 2.0
import org.kde.discover.app 1.0
import QtTest 1.1

DiscoverTest
{
    onReset: {
        appRoot.currentTopLevel = appRoot.topBrowsingComp
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
        compare(typeName(updatePage), "UpdatesPage")
        compare(updatePage.state, "has-updates", "to update")
        var button = findChild(updatePage, "Button")
        verify(button);
        button.clicked();
        compare(updatePage.state, "has-updates", "updating")

        //make sure the window doesn't close while updating
        verify(appRoot.visible);
        verify(waitForRendering())
        appRoot.close()
        verify(appRoot.visible);

        while(ResourcesModel.updatesCount>0)
            verify(waitForSignal(ResourcesModel, "updatesCountChanged"))
        compare(updatePage.state, "now-uptodate", "update finished")
        compare(ResourcesModel.updatesCount, 0, "should be up to date")
    }

    function test_search() {
        app.openMode("Browsing");
        var searchField = findChild(appRoot.globalDrawer, "TextField");
        verify(searchField);
        searchField.text = "cocacola"
        verify(waitForSignal(appRoot.stack, "currentItemChanged"))
        while(!isType(appRoot.stack.currentItem, "ApplicationsListPage"))
            verify(waitForSignal(appRoot.stack, "currentItemChanged"))
        var listPage = appRoot.stack.currentItem
        compare(listPage.count, 0)
        verify(waitForSignal(listPage, "searchChanged"))
        compare(listPage.search, "cocacola")
        searchField.text = "dummy"
        verify(waitForSignal(listPage, "searchChanged"))
        compare(listPage.search, searchField.text)
        compare(listPage.count, ResourcesModel.rowCount()/2)
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
