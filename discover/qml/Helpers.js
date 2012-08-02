function getFeatured(model, data) {
    if(data==null)
        return
    
    for(var packageName in data) {
        var currentData = data[packageName]
        var resource = resourcesModel.resourceByPackageName(currentData.package)
        if(resource==null) {
//             console.log("resource "+ currentData.package+" not found")
            continue
        }
        var image = currentData.image
        if(image == null)
            image = resource.screenshotUrl
        model.append({
            "text": resource.name,
            "color": "red",
            "image": image,
            "icon": resource.icon,
            "comment": resource.comment,
            "packageName": currentData.package })
    }
}