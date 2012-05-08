function getFeatured(model, data) {
    if(!app.appBackend || data==null)
        return
    
    for(var packageName in data) {
        var application = app.appBackend.applicationByPackageName(data[packageName].package)
        if(!application) {
            console.log("application "+data[packageName].package+" not found")
            continue
        }
        model.append({
            "text": application.name,
            "color": "blue",
            "icon": application.icon,
            "packageName": data[packageName].package })
    }
}