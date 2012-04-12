import QtQuick 1.1

ListModel {
    id: model
    property QtObject application
    Component.onCompleted: {
//         if(application.screenshotUrl(0).indexOf("http://screenshots.debian.net")<0)
//             append({
//             'small_image_url': application.screenshotUrl(0),
//             'large_image_url': application.screenshotUrl(1)
//         })
        
        var xhr = new XMLHttpRequest;
        xhr.open("GET", "http://screenshots.debian.net/json/package/"+application.packageName);
        xhr.onreadystatechange = function() {
            if (xhr.readyState == XMLHttpRequest.DONE) {
                var data = JSON.parse(xhr.responseText)
                for(var app in data.screenshots) {
                    model.append(data.screenshots[app])
                }
            }
        }
        xhr.send();
    }
}