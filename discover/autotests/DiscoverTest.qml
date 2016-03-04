import QtQuick 2.1
import QtTest 1.1

Item
{
    id: testRoot
    property QtObject appRoot

    signal reset()

    function verify(condition, msg) {
        if (!condition) {
            console.trace();
            var e = new Error(msg)
            e.object = testRoot;
            throw e;
        }
    }

    function compare(valA, valB, msg) {
        if (valA !== valB) {
            console.trace();
            var e = new Error(valA + " !== " + valB + ": " + msg)
            e.object = testRoot;
            throw e;
        }
    }

    function findChild(obj, typename) {
        if (obj.toString().indexOf(typename+"_QMLTYPE_") == 0)
            return obj;

        for(var v in obj.children) {
            var v = findChild(obj.children[v], typename)
            if (v)
                return v
        }
        return null
    }

    SignalSpy {
        id: spy
    }

    function waitForSignal(object, name) {
        spy.signalName = ""
        spy.target = object;
        spy.signalName = name;
        verify(spy);

        var done = true;
        try {
            spy.wait(5000);
        } catch (e) {
            done = false;
        }
        return done;
    }

    function waitForRendering() {
        return waitForSignal(app, "frameSwapped")
    }

    Connections {
        target: ResourcesModel
        onIsFetchingChanged: {
            if (ResourcesModel.isFetching)
                return;

            for(var v in testRoot) {
                if (v.indexOf("test_") == 0) {
                    testRoot.reset();
                    testRoot[v]();
                }
            }
            Qt.quit();
        }
    }
}
