/*
 *   Copyright (C) 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

function openApplicationList(cat, search) {
    openPage(applicationListComp, { category: cat, search: search, preferList: search!="" })
}

function openApplicationListSource(origin) {
    openPage(applicationListComp, { originFilter: origin, preferList: true, title: origin, icon: "view-filter" })
}

function openApplicationMime(mime) {
    openPage(applicationListComp, { mimeTypeFilter: mime , icon: "document-open-data", title: i18n("Resources for '%1'", mime) })
}

function openCategoryByName(catname) {
    currentTopLevel = topBrowsingComp
    openCategory(window.stack.currentItem.categories.findCategoryByName(catname))
}

function openCategory(cat) {
    if(cat.hasSubCategories)
        openPage(categoryComp, { category: cat })
    else
        openApplicationList(cat, "")
}

function openApplication(app) {
    openPage(applicationComp, { application: app })
}

function openReviews(app, reviews) {
    openPage(reviewsComp, { model: reviews, title: i18n("Ratings for %1", app.name), icon: "rating" })
}

function openPage(component, props) {
    var obj
    try {
        obj = component.createObject(window.stack.currentItem, props)
        window.stack.push(obj);
        if (!obj)
            console.log("error opening", name, obj, component.errorString())
    } catch (e) {
        console.log("error: "+e)
        console.log("comp error: "+component.errorString())
    }
    return obj
}
