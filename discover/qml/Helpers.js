function getFeatured(model, data) {
    if(!app.appBackend || data==null)
        return
    
    for(var packageName in data) {
        var application = app.appBackend.applicationByPackageName(data[packageName].package)
        var currentData = data[packageName]
        if(!application) {
            console.log("application "+ currentData.package+" not found")
            continue
        }
        var image = currentData.image
        if(image == null)
            image = application.screenshotUrl
        model.append({
            "text": application.name,
            "color": "red",
            "image": image,
            "icon": application.icon,
            "comment": application.comment,
            "packageName": currentData.package })
    }
}