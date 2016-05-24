import QtQuick 2.1
import QtQuick.Controls 1.4
import QtTest 1.1
import org.kde.discover.app 1.0

Item
{
    id: testRoot

    signal reset()
    property QtObject appRoot

    StackViewDelegate {
        id: noTransitionsDelegate
        popTransition: StackViewTransition { immediate: true }
        pushTransition: StackViewTransition { immediate: true }
        replaceTransition: StackViewTransition { immediate: true }
    }

    onAppRootChanged: {
        if (!appRoot)
            return;
        appRoot.stack.delegate = noTransitionsDelegate
    }

    function verify(condition, msg) {
        if (!condition) {
            console.trace();
            var e = new Error(condition + (msg ? (": " + msg) : ""))
            e.object = testRoot;
            throw e;
        }
    }

    function compare(valA, valB, msg) {
        if (valA !== valB) {
            console.trace();
            var e = new Error(valA + " !== " + valB + (msg ? (": " + msg) : ""))
            e.object = testRoot;
            throw e;
        }
    }

    function typeName(obj) {
        var name = obj.toString();
        var idx = name.indexOf("_QMLTYPE_");
        return name.substring(0, idx);
    }

    function isType(obj, typename) {
        return obj && obj.toString().indexOf(typename+"_QMLTYPE_") == 0
    }

    function findChild(obj, typename) {
        if (isType(obj, typename))
            return obj;
        for(var v in obj.data) {
            var v = findChild(obj.data[v], typename)
            if (v)
                return v
        }
        return null
    }

    SignalSpy {
        id: spy
    }

    function waitForSignal(object, name, timeout) {
        if (!timeout) timeout = 5000;

        spy.signalName = ""
        spy.target = object;
        spy.signalName = name;
        verify(spy);

        try {
            spy.wait(timeout);
        } catch (e) {
            console.warn("wait for signal unsuccessful")
            return false;
        }
        return spy.count>0;
    }

    function waitForRendering() {
        return waitForSignal(Helpers.mainWindow, "frameSwapped")
    }

    property string currentTest: "<null>"
    onCurrentTestChanged: console.log("changed to test", currentTest)

    Connections {
        target: ResourcesModel
        property bool done: false
        onIsFetchingChanged: {
            if (ResourcesModel.isFetching)
                return;

            done = true;
            for(var v in testRoot) {
                if (v.indexOf("test_") == 0) {
                    testRoot.currentTest = v;
                    testRoot.reset();
                    testRoot[v]();
                }
            }
            Qt.quit();
        }
    }
}
