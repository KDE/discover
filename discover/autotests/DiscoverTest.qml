import QtQuick 2.1

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
