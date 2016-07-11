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

function openApplicationListSource(origin) {
    openApplicationList({ originFilter: origin, preferList: true, title: origin, icon: "view-filter" })
}

function openApplicationMime(mime) {
    openApplicationList({ mimeTypeFilter: mime , icon: "document-open-data", title: i18n("Resources for '%1'", mime) })
}

function openApplicationList(props) {
    window.stack.clear();
    window.stack.push(applicationListComp, props)
}

function openCategory(cat) {
    openApplicationList({ category: cat })
}

function openApplication(app) {
    window.stack.push(applicationComp, { application: app })
}

function openReviews(app, reviews) {
    window.stack.push(reviewsComp, { model: reviews, title: i18n("Ratings for %1", app.name), icon: "rating" })
}

function openExtends(ext) {
    window.stack.push(applicationListComp, { extend: ext, title: i18n("Extensions...") })
}
