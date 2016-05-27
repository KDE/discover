import QtQuick 2.1
import org.kde.discover 1.0

ListModel
{
    id: model
    Component.onCompleted: {
        fetchSource(app.prioritaryFeaturedSource)
        fetchSource(app.featuredSource)
    }

    function fetchSource(source)
    {
        if(source=="")
            return
        var xhr = new XMLHttpRequest;
        xhr.open("GET", source);
        xhr.onreadystatechange = function() {
            if (xhr.readyState == XMLHttpRequest.DONE) {
                try {
                    getFeatured(JSON.parse(xhr.responseText))
                } catch (e) {
                    console.log("json error", e, xhr.responseText)
                }
            }
        }
        xhr.send();
    }

    readonly property var fu: Connections {
        target: ResourcesModel
        onAllInitialized: featuredModel.initFeatured(true)
    }
    
    property var dataFound: []

    function initFeatured() {
        for(var i in dataFound) {
            var data = dataFound[i];
            if (!data)
                continue;
            if(data.packageName) {
                var appl = ResourcesModel.resourceByPackageName(data.packageName)
                if(appl==null) {
                    console.log("application ", data.packageName, " not found")

                    continue
                }
                if(data.image==null)
                    data.image = appl.screenshotUrl
                data.text = appl.name
                data.icon = appl.icon
                data.comment = appl.comment
            }
            model.append(data)
        }
    }

    function alternateIfNull(valueA, valueB)
    {
        return valueA!=null ? valueA : valueB;
    }

    function getFeatured(data) {
        if(data==null)
            return

        var objs = []
        for(var packageName in data) {
            var currentData = data[packageName]
            objs.push({
                "text": alternateIfNull(currentData.text, currentData.package),
                "color": alternateIfNull(currentData.color, "red"),
                "image": currentData.image,
                "icon": alternateIfNull(currentData.icon, "kde"),
                "comment": alternateIfNull(currentData.comment, "&nbsp;"),
                "packageName": alternateIfNull(currentData.package, ""),
                "url": currentData.url
            })
        }
        dataFound = dataFound.concat(objs);
        initFeatured();
    }
}
