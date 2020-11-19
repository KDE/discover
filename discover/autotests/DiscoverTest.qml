import QtQuick 2.1
import QtTest 1.1
import org.kde.discover.app 1.0

Item
{
    id: testRoot

    signal reset()
    property QtObject appRoot

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
        return obj && obj.toString().indexOf(typename+"_QMLTYPE_") === 0
    }

    function chooseChildren(objects, validator) {
        for (var v in objects) {
            var obj = objects[v];
            if (validator(obj))
                return true;
        }
        return false;
    }

    function chooseChild(obj, validator) {
        verify(obj, "can't find a null's child")
        if (validator(obj))
            return true;
        var children = obj.data ? obj.data : obj.contentData
        for(var v in children) {
            var stop = chooseChild(children[v], validator)
            if (stop)
                return true
        }
        return false
    }

    function findChild(obj, typename) {
        var ret = null;
        chooseChild(obj, function(o) {
            var found = isType(o, typename);
            if (found) {
                ret = o;
            }
            return found
        })
        return ret;
    }

    SignalSpy {
        id: spy
    }

    function waitForSignal(object, name, timeout) {
        if (!timeout) timeout = 5000;

        spy.clear();
        spy.signalName = ""
        spy.target = object;
        spy.signalName = name;
        verify(spy);
        verify(spy.valid);
        verify(spy.count == 0);

        try {
            spy.wait(timeout);
        } catch (e) {
            console.warn("wait for signal '"+name+"' failed")
            return false;
        }
        return spy.count>0;
    }

    function waitForRendering() {
        return waitForSignal(appRoot, "frameSwapped")
    }

    property string currentTest: "<null>"
    onCurrentTestChanged: console.log("changed to test", currentTest)

    Connections {
        target: ResourcesModel
        property bool done: false
        function onIsFetchingChanged() {
            if (ResourcesModel.isFetching || done)
                return;

            done = true;
            for(var v in testRoot) {
                if (v.indexOf("test_") === 0) {
                    console.log("doing", v)
                    testRoot.currentTest = v;
                    testRoot.reset();
                    testRoot[v]();
                }
            }
            console.log("done")
            appRoot.close()
        }
    }
}
