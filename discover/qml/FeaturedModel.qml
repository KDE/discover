import QtQuick 2.1
import org.kde.discover 1.0

ListModel
{
    id: model
    Component.onCompleted: {
        fetchSource(app.prioritaryFeaturedSource)
        fetchSource(app.featuredSource)
    }
    
    property variant fu: Connections {
        target: ResourcesModel
        onRowsInserted: initFeatured()
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
    
    function initFeatured() {
        for(var row=0; row<model.count; row++) {
            var data = model.get(row)
            if(data.packageName) {
                var appl = ResourcesModel.resourceByPackageName(data.packageName)
                if(appl==null) {
//                     console.log("application ", data.packageName, " not found")
                    continue
                }
                if(data.image==null)
                    data.image = appl.screenshotUrl
                data.text = appl.name
                data.icon = appl.icon
                data.comment = appl.comment
            }
            model.set(row, data)
        }
    }

    function alternateIfNull(valueA, valueB)
    {
        return valueA!=null ? valueA : valueB;
    }

    function getFeatured(data) {
        if(data==null)
            return
        
        for(var packageName in data) {
            var currentData = data[packageName]
            model.append({
                "text": alternateIfNull(currentData.text, currentData.package),
                "color": alternateIfNull(currentData.color, "red"),
                "image": currentData.image,
                "icon": alternateIfNull(currentData.icon, "kde"),
                "comment": alternateIfNull(currentData.comment, "&nbsp;"),
                "packageName": currentData.package,
                "url": currentData.url
            })
        }
        initFeatured()
    }
}
