import QtQuick 2.1
import QtTest 1.1

QtObject
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

    readonly property var ssc: Component {
        id: signalSpyComponent
        SignalSpy {}
    }

    function waitForSignal(object, name) {
        var spy = signalSpyComponent.createObject(null, {
            target: object,
            signalName: name
        });
        verify(spy);

        spy.wait(10000);
        spy.destroy();
    }

    readonly property var conex: Connections {
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
