function getFeatured(model, data) {
    if(data==null)
        return
    
    for(var packageName in data) {
        var currentData = data[packageName]
        var application = resourcesModel.applicationByPackageName(currentData.package)
        if(application==null) {
//             console.log("application "+ currentData.package+" not found")
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