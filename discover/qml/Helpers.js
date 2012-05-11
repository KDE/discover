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
        model.append({
            "text": application.name,
            "color": "red",
            "image": currentData.image,
            "icon": application.icon,
            "packageName": currentData.package })
    }
}