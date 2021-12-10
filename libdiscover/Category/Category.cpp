/*
 *   SPDX-FileCopyrightText: 2010 Jonathan Thomas <echidnaman@kubuntu.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "Category.h"

#include <QDomNode>

#include "libdiscover_debug.h"
#include <KLocalizedString>
#include <QFile>
#include <QStandardPaths>
#include <utils.h>

Category::Category(QSet<QString> pluginName, QObject *parent)
    : QObject(parent)
    , m_iconString(QStringLiteral("applications-other"))
    , m_plugins(std::move(pluginName))
{
}

Category::Category(const QString &name,
                   const QString &iconName,
                   const QVector<QPair<FilterType, QString>> &orFilters,
                   const QSet<QString> &pluginName,
                   const QVector<Category *> &subCategories,
                   const QUrl &decoration,
                   bool isAddons)
    : QObject(nullptr)
    , m_name(name)
    , m_iconString(iconName)
    , m_decoration(decoration)
    , m_orFilters(orFilters)
    , m_subCategories(subCategories)
    , m_plugins(pluginName)
    , m_isAddons(isAddons)
{
    setObjectName(m_name);
}

Category::~Category() = default;

void Category::parseData(const QString &path, const QDomNode &data)
{
    for (QDomNode node = data.firstChild(); !node.isNull(); node = node.nextSibling()) {
        if (!node.isElement()) {
            if (!node.isComment())
                qCWarning(LIBDISCOVER_LOG) << "unknown node found at " << QStringLiteral("%1:%2").arg(path).arg(node.lineNumber());
            continue;
        }
        QDomElement tempElement = node.toElement();

        if (tempElement.tagName() == QLatin1String("Name")) {
            m_name = i18nc("Category", tempElement.text().toUtf8().constData());
            setObjectName(m_name);
        } else if (tempElement.tagName() == QLatin1String("Menu")) {
            m_subCategories << new Category(m_plugins, this);
            m_subCategories.last()->parseData(path, node);
        } else if (tempElement.tagName() == QLatin1String("Image")) {
            m_decoration = QUrl(tempElement.text());
            if (m_decoration.isRelative()) {
                m_decoration =
                    QUrl::fromLocalFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("discover/") + tempElement.text()));
                if (m_decoration.isEmpty())
                    qCWarning(LIBDISCOVER_LOG) << "couldn't find category decoration" << tempElement.text();
            }
        } else if (tempElement.tagName() == QLatin1String("Addons")) {
            m_isAddons = true;
        } else if (tempElement.tagName() == QLatin1String("Icon") && tempElement.hasChildNodes()) {
            m_iconString = tempElement.text();
        } else if (tempElement.tagName() == QLatin1String("Include")) { // previous muon format
            parseIncludes(tempElement);
        } else if (tempElement.tagName() == QLatin1String("Categories")) { // as provided by appstream
            parseIncludes(tempElement);
        }
    }
}

QVector<QPair<FilterType, QString>> Category::parseIncludes(const QDomNode &data)
{
    QDomNode node = data.firstChild();
    QVector<QPair<FilterType, QString>> filter;
    while (!node.isNull()) {
        QDomElement tempElement = node.toElement();

        if (tempElement.tagName() == QLatin1String("And")) {
            // Parse children
            m_andFilters.append(parseIncludes(node));
        } else if (tempElement.tagName() == QLatin1String("Or")) {
            m_orFilters.append(parseIncludes(node));
        } else if (tempElement.tagName() == QLatin1String("Not")) {
            m_notFilters.append(parseIncludes(node));
        } else if (tempElement.tagName() == QLatin1String("PkgSection")) {
            filter.append({PkgSectionFilter, tempElement.text()});
        } else if (tempElement.tagName() == QLatin1String("Category")) {
            filter.append({CategoryFilter, tempElement.text()});
        } else if (tempElement.tagName() == QLatin1String("PkgWildcard")) {
            filter.append({PkgWildcardFilter, tempElement.text()});
        } else if (tempElement.tagName() == QLatin1String("AppstreamIdWildcard")) {
            filter.append({AppstreamIdWildcardFilter, tempElement.text()});
        } else if (tempElement.tagName() == QLatin1String("PkgName")) {
            filter.append({PkgNameFilter, tempElement.text()});
        } else {
            qCWarning(LIBDISCOVER_LOG) << "unknown" << tempElement.tagName();
        }
        node = node.nextSibling();
    }

    return filter;
}

QString Category::name() const
{
    return m_name;
}

void Category::setName(const QString &name)
{
    m_name = name;
    Q_EMIT nameChanged();
}

QString Category::icon() const
{
    return m_iconString;
}

QVector<QPair<FilterType, QString>> Category::andFilters() const
{
    return m_andFilters;
}

void Category::setAndFilter(const QVector<QPair<FilterType, QString>> &filters)
{
    m_andFilters = filters;
}

QVector<QPair<FilterType, QString>> Category::orFilters() const
{
    return m_orFilters;
}

QVector<QPair<FilterType, QString>> Category::notFilters() const
{
    return m_notFilters;
}

QVector<Category *> Category::subCategories() const
{
    return m_subCategories;
}

bool Category::categoryLessThan(Category *c1, const Category *c2)
{
    return (!c1->isAddons() && c2->isAddons()) || (c1->isAddons() == c2->isAddons() && QString::localeAwareCompare(c1->name(), c2->name()) < 0);
}

static bool isSorted(const QVector<Category *> &vector)
{
    Category *last = nullptr;
    for (auto a : vector) {
        if (last && !Category::categoryLessThan(last, a))
            return false;
        last = a;
    }
    return true;
}

void Category::sortCategories(QVector<Category *> &cats)
{
    std::sort(cats.begin(), cats.end(), &categoryLessThan);
    for (auto cat : cats) {
        sortCategories(cat->m_subCategories);
    }
    Q_ASSERT(isSorted(cats));
}

void Category::addSubcategory(QVector<Category *> &list, Category *newcat)
{
    Q_ASSERT(isSorted(list));

    auto it = std::lower_bound(list.begin(), list.end(), newcat, &categoryLessThan);
    if (it == list.end()) {
        list << newcat;
        return;
    }

    auto c = *it;
    if (c->name() == newcat->name()) {
        if (c->icon() != newcat->icon() || c->m_andFilters != newcat->m_andFilters || c->m_isAddons != newcat->m_isAddons) {
            qCWarning(LIBDISCOVER_LOG) << "the following categories seem to be the same but they're not entirely" << c->icon() << newcat->icon() << "--"
                                       << c->name() << newcat->name() << "--" << c->andFilters() << newcat->andFilters() << "--" << c->isAddons()
                                       << newcat->isAddons();
        } else {
            c->m_orFilters += newcat->orFilters();
            c->m_notFilters += newcat->notFilters();
            c->m_plugins.unite(newcat->m_plugins);
            const auto subCategories = newcat->subCategories();
            for (Category *nc : subCategories) {
                addSubcategory(c->m_subCategories, nc);
            }
            return;
        }
    }

    list.insert(it, newcat);
    Q_ASSERT(isSorted(list));
}

void Category::addSubcategory(Category *cat)
{
    int i = 0;
    for (Category *subCat : qAsConst(m_subCategories)) {
        if (!categoryLessThan(subCat, cat)) {
            break;
        }
        ++i;
    }
    m_subCategories.insert(i, cat);
    Q_ASSERT(isSorted(m_subCategories));
}

bool Category::blacklistPluginsInVector(const QSet<QString> &pluginNames, QVector<Category *> &subCategories)
{
    bool ret = false;
    for (QVector<Category *>::iterator it = subCategories.begin(); it != subCategories.end();) {
        if ((*it)->blacklistPlugins(pluginNames)) {
            delete *it;
            it = subCategories.erase(it);
            ret = true;
        } else
            ++it;
    }
    return ret;
}

bool Category::blacklistPlugins(const QSet<QString> &pluginNames)
{
    if (m_plugins.subtract(pluginNames).isEmpty()) {
        return true;
    }

    if (blacklistPluginsInVector(pluginNames, m_subCategories))
        Q_EMIT subCategoriesChanged();
    return false;
}

QUrl Category::decoration() const
{
    if (m_decoration.isEmpty()) {
        Category *c = qobject_cast<Category *>(parent());
        return c ? c->decoration() : QUrl();
    } else {
        Q_ASSERT(!m_decoration.isLocalFile() || QFile::exists(m_decoration.toLocalFile()));
        return m_decoration;
    }
}

QVariantList Category::subCategoriesVariant() const
{
    return kTransform<QVariantList>(m_subCategories, [](Category *cat) {
        return QVariant::fromValue<QObject *>(cat);
    });
}

bool Category::matchesCategoryName(const QString &name) const
{
    for (const auto &filter : m_orFilters) {
        if (filter.first == CategoryFilter && filter.second == name)
            return true;
    }
    return false;
}

bool Category::contains(Category *cat) const
{
    const bool ret = cat == this || (cat && contains(qobject_cast<Category *>(cat->parent())));
    return ret;
}

bool Category::contains(const QVariantList &cats) const
{
    bool ret = false;
    for (const auto &itCat : cats) {
        if (contains(qobject_cast<Category *>(itCat.value<QObject *>()))) {
            ret = true;
            break;
        }
    }
    return ret;
}
