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

    function test_openHome() {
        var drawer = appRoot.globalDrawer;
        drawer.actions[0].children[2].trigger()
        compare(appRoot.stack.currentItem.title, "dummy 3", "same title");

        app.openMode("Browsing");

        compare(appRoot.stack.currentItem.title, "Featured", "same title");
        compare(drawer.currentSubMenu, null)
    }

    function test_navigateThenUpdate() {
        var drawer = appRoot.globalDrawer;
        var firstitem = drawer.actions[0].children[2]
        var updateButton;
        chooseChild(drawer, function(object) {
            if (object.objectName === "updateButton") {
                updateButton = object;
                return true
            }
            return false;
        });

        firstitem.trigger()
        verify(updateButton.enabled)
        updateButton.clicked()

        compare(appRoot.currentTopLevel, appRoot.topUpdateComp, "correct component, updates");
    }

    function test_update() {
        app.openMode("Update");

        var updatePage = appRoot.stack.currentItem;
        compare(typeName(updatePage), "UpdatesPage")
        compare(updatePage.state, "has-updates", "to update")
        var action = updatePage.actions.main
        verify(action);
        action.triggered(updatePage);
        compare(updatePage.state, "progressing", "updating")

        //make sure the window doesn't close while updating
        verify(appRoot.visible);
        verify(waitForRendering())
        appRoot.close()
        verify(appRoot.visible);

        while(updatePage.state !== "now-uptodate")
            waitForSignal(updatePage, "stateChanged")
        compare(ResourcesModel.updatesCount, 0, "should be up to date")
    }

    function test_search() {
        app.openMode("Browsing");
        app.openSearch("cocacola")
        while(!isType(appRoot.stack.currentItem, "ApplicationsListPage"))
            verify(waitForSignal(appRoot.stack, "currentItemChanged"))
        var listPage = appRoot.stack.currentItem
        while(listPage.count>0)
            verify(waitForSignal(listPage, "countChanged"))
        compare(listPage.count, 0)
        compare(listPage.search, "cocacola")
        app.openSearch("dummy")
        listPage = appRoot.stack.currentItem
        compare(listPage.search, "dummy")
//         compare(listPage.count, ResourcesModel.rowCount()/2)
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
