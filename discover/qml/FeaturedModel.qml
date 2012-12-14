import QtQuick 1.1

ListModel
{
    id: model
    Component.onCompleted: {
        fetchSource(app.prioritaryFeaturedSource())
        fetchSource(app.featuredSource())
    }
    
    property variant fu: Connections {
        target: resourcesModel
        onRowsInserted: initFeatured()
    }
    
    function fetchSource(source)
    {
        var xhr = new XMLHttpRequest;
        xhr.open("GET", source);
        xhr.onreadystatechange = function() {
            if (xhr.readyState == XMLHttpRequest.DONE) {
                getFeatured(JSON.parse(xhr.responseText))
            }
        }
        xhr.send();
    }
    
    function initFeatured() {
        for(var row=0; row<model.count; row++) {
            var data = model.get(row)
            if(data.packageName) {
                var appl = resourcesModel.resourceByPackageName(data.packageName)
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


    function getFeatured(data) {
        if(data==null)
            return
        
        for(var packageName in data) {
            var currentData = data[packageName]
            model.append({
                "text": currentData.package,
                "color": "red",
                "image": currentData.image,
                "icon": "kde",
                "comment": "&nbsp;",
                "packageName": currentData.package
            })
        }
        initFeatured()
    }
}