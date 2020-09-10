/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

function clearStack()
{
    window.currentTopLevel = ""
    window.stack.clear();
}

function openApplicationListSource(origin) {
    openApplicationList({ originFilter: origin, title: origin, allBackends: true })
}

function openApplicationMime(mime) {
    clearStack()
    openApplicationList({ mimeTypeFilter: mime, title: i18n("Resources for '%1'", mime) })
}

function openApplicationList(props) {
    var page = window.stack.push("qrc:/qml/ApplicationsListPage.qml", props)
    if (props.search === "")
        page.clearSearch();
}

function openCategory(cat, search) {
    clearStack()
    openApplicationList({ category: cat, search: search })
}

function openApplication(app) {
    console.assert(app)
    window.stack.push("qrc:/qml/ApplicationPage.qml", { application: app })
}

function openReviews(model) {
    window.stack.push("qrc:/qml/ReviewsPage.qml", { model: model })
}

function openExtends(ext) {
    window.stack.push("qrc:/qml/ApplicationsListPage.qml", { extending: ext, title: i18n("Extensions...") })
}

function openHome() {
    if (window.globalDrawer.currentSubMenu)
        window.globalDrawer.resetMenu();
    clearStack()
    var page = window.stack.push(topBrowsingComp)
    page.clearSearch()
}
