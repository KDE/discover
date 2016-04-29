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
    window.stack.push(applicationListComp, { category: cat, search: search, preferList: search!="" })
}

function openApplicationListSource(origin) {
    window.stack.push(applicationListComp, { originFilter: origin, preferList: true, title: origin, icon: "view-filter" })
}

function openApplicationMime(mime) {
    window.stack.push(applicationListComp, { mimeTypeFilter: mime , icon: "document-open-data", title: i18n("Resources for '%1'", mime) })
}

function openCategory(cat) {
    window.stack.push(categoryComp, { category: cat })
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
