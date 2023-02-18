/*
 *   SPDX-FileCopyrightText: 2010 Jonathan Thomas <echidnaman@kubuntu.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "Category.h"

#include <QTimer>

#include "libdiscover_debug.h"
#include <KLocalizedString>
#include <QFile>
#include <QStandardPaths>
#include <QXmlStreamReader>
#include <utils.h>

Category::Category(QSet<QString> pluginName, QObject *parent)
    : QObject(parent)
    , m_iconString(QStringLiteral("applications-other"))
    , m_plugins(std::move(pluginName))
{
    // Use a timer so to compress the rootCategoriesChanged signal
    // It generally triggers when KNS is unavailable at large (as explained in bug 454442)
    m_subCategoriesChanged = new QTimer(this);
    m_subCategoriesChanged->setInterval(0);
    m_subCategoriesChanged->setSingleShot(true);
    connect(m_subCategoriesChanged, &QTimer::timeout, this, &Category::subCategoriesChanged);
}

Category::Category(const QString &name,
                   const QString &iconName,
                   const CategoryFilter &filter,
                   const QSet<QString> &pluginName,
                   const QVector<Category *> &subCategories,
                   bool isAddons)
    : QObject(nullptr)
    , m_name(name)
    , m_iconString(iconName)
    , m_filter(filter)
    , m_subCategories(subCategories)
    , m_plugins(pluginName)
    , m_isAddons(isAddons)
    , m_priority(isAddons ? 5 : 0)
{
    setObjectName(m_name);

    // Use a timer so to compress the rootCategoriesChanged signal
    // It generally triggers when KNS is unavailable at large (as explained in bug 454442)
    m_subCategoriesChanged = new QTimer(this);
    m_subCategoriesChanged->setInterval(0);
    m_subCategoriesChanged->setSingleShot(true);
    connect(m_subCategoriesChanged, &QTimer::timeout, this, &Category::subCategoriesChanged);
}

Category::~Category() = default;

void Category::parseData(const QString &path, QXmlStreamReader *xml)
{
    Q_ASSERT(xml->name() == QLatin1String("Menu"));
    while (!xml->atEnd() && !xml->hasError()) {
        xml->readNext();

        if (xml->isEndElement() && xml->name() == QLatin1String("Menu")) {
            break;
        } else if (!xml->isStartElement()) {
            if (xml->isCharacters() && xml->text().trimmed().isEmpty())
                ;
            else if (!xml->isComment())
                qDebug() << "skipping" << xml->tokenString() << xml->name();
            continue;
        }

        if (xml->name() == QLatin1String("Name")) {
            m_untranslatedName = xml->readElementText();
            m_name = i18nc("Category", m_untranslatedName.toUtf8().constData());
            setObjectName(m_untranslatedName);
        } else if (xml->name() == QLatin1String("Menu")) {
            m_subCategories << new Category(m_plugins, this);
            m_subCategories.last()->parseData(path, xml);
        } else if (xml->name() == QLatin1String("Addons")) {
            m_isAddons = true;
            m_priority = 5;
            xml->readNext();
        } else if (xml->name() == QLatin1String("Icon")) {
            m_iconString = xml->readElementText();
        } else if (xml->name() == QLatin1String("Include")
                   || xml->name() == QLatin1String("Categories")) {
            const QString opening = xml->name().toString();
            while (!xml->atEnd() && !xml->hasError()) {
                xml->readNext();

                if (xml->isEndElement() && xml->name() == opening) {
                    qDebug() << "weird, let's go" << opening << xml->lineNumber();
                    break;
                } else if (!xml->isStartElement()) {
                    if (xml->isCharacters() && xml->text().trimmed().isEmpty())
                        ;
                    else if (!xml->isComment())
                        qDebug() << "include skipping" << xml->tokenString() << xml->text() << xml->name() << opening << xml->lineNumber();
                    continue;
                }
                break;
            }
            m_filter = parseIncludes(xml);

            // Here we are at the end of the last item in the group, we need to finish what we started
            while (!xml->atEnd() && !xml->hasError()) {
                xml->readNext();

                if (xml->isEndElement() && xml->name() == opening) {
                    break;
                } else {
                    if (xml->isCharacters() && xml->text().trimmed().isEmpty())
                        ;
                    else if (!xml->isComment())
                        qDebug() << "include2 skipping" << xml->tokenString() << xml->text() << xml->name() << opening << xml->lineNumber();
                    continue;
                }
                break;
            }
        } else if (xml->name() == QLatin1String("Top")) {
            xml->skipCurrentElement();
            m_priority = -5;
        } else {
            qDebug() << "unknown element" << xml->name();
            xml->skipCurrentElement();
        }
        Q_ASSERT(xml->isEndElement());
    }
    Q_ASSERT(xml->isEndElement() && xml->name() == QLatin1String("Menu"));
}

CategoryFilter Category::parseIncludes(QXmlStreamReader *xml)
{
    const QString opening = xml->name().toString();
    Q_ASSERT(xml->isStartElement());

    auto subIncludes = [&]() {
        QVector<CategoryFilter> filters;

        Q_ASSERT(xml->isStartElement());
        const QString opening = xml->name().toString();

        while (!xml->atEnd() && !xml->hasError()) {
            xml->readNext();

            if (xml->isEndElement()) {
                break;
            } else if (xml->isStartElement()) {
                filters.append(parseIncludes(xml));
            }
        }
        Q_ASSERT(xml->isEndElement());
        Q_ASSERT(xml->name() == opening);
        return filters;
    };

    CategoryFilter filter;
    if (xml->name() == QLatin1String("And")) {
        filter = {CategoryFilter::AndFilter, subIncludes()};
    } else if (xml->name() == QLatin1String("Or")) {
        filter = {CategoryFilter::OrFilter, subIncludes()};
    } else if (xml->name() == QLatin1String("Not")) {
        filter = {CategoryFilter::NotFilter, subIncludes()};
    } else if (xml->name() == QLatin1String("PkgSection")) {
        filter = {CategoryFilter::PkgSectionFilter, xml->readElementText()};
    } else if (xml->name() == QLatin1String("Category")) {
        filter = {CategoryFilter::CategoryNameFilter, xml->readElementText()};
        Q_ASSERT(xml->isEndElement() && xml->name() == QLatin1String("Category"));
    } else if (xml->name() == QLatin1String("PkgWildcard")) {
        filter = {CategoryFilter::PkgWildcardFilter, xml->readElementText()};
    } else if (xml->name() == QLatin1String("AppstreamIdWildcard")) {
        filter = {CategoryFilter::AppstreamIdWildcardFilter, xml->readElementText()};
    } else if (xml->name() == QLatin1String("PkgName")) {
        filter = {CategoryFilter::PkgNameFilter, xml->readElementText()};
    } else {
        qCWarning(LIBDISCOVER_LOG) << "unknown" << xml->name() << xml->lineNumber();
    }

    Q_ASSERT(xml->isEndElement());
    Q_ASSERT(xml->name() == opening);

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

CategoryFilter Category::filter() const
{
    return m_filter;
}

void Category::setFilter(const CategoryFilter &filter)
{
    m_filter = filter;
}

QVector<Category *> Category::subCategories() const
{
    return m_subCategories;
}

bool Category::categoryLessThan(Category *c1, const Category *c2)
{
    return (c1->priority() < c2->priority()) || (c1->priority() == c2->priority() && QString::localeAwareCompare(c1->name(), c2->name()) < 0);
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

QDebug operator<<(QDebug debug, const CategoryFilter &filter)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "Filter(";
    debug << filter.type << ", ";

    if (auto x = std::get_if<QString>(&filter.value)) {
        debug << std::get<QString>(filter.value);
    } else {
        debug << std::get<QVector<CategoryFilter>>(filter.value);
    }
    debug.nospace() << ')';
    return debug;
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
        if (c->icon() != newcat->icon() || c->m_priority != newcat->m_priority) {
            qCWarning(LIBDISCOVER_LOG) << "the following categories seem to be the same but they're not entirely" << c->icon() << newcat->icon() << "--"
                                       << c->name() << newcat->name() << "--" << c->isAddons() << newcat->isAddons();
        } else {
            CategoryFilter newFilter = {CategoryFilter::OrFilter, QVector<CategoryFilter>{c->m_filter, newcat->m_filter}};
            c->m_filter = newFilter;
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
    for (Category *subCat : std::as_const(m_subCategories)) {
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

    if (blacklistPluginsInVector(pluginNames, m_subCategories)) {
        m_subCategoriesChanged->start();
    }
    return false;
}

QVariantList Category::subCategoriesVariant() const
{
    return kTransform<QVariantList>(m_subCategories, [](Category *cat) {
        return QVariant::fromValue<QObject *>(cat);
    });
}

bool Category::matchesCategoryName(const QString &name) const
{
    return involvedCategories().contains(name);
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

static QStringList involvedCategories(const CategoryFilter &f)
{
    switch (f.type) {
    case CategoryFilter::CategoryNameFilter:
        return {std::get<QString>(f.value)};
    case CategoryFilter::OrFilter:
    case CategoryFilter::AndFilter: {
        const auto filters = std::get<QVector<CategoryFilter>>(f.value);
        QStringList ret;
        ret.reserve(filters.size());
        for (const auto &subFilters : filters) {
            ret << involvedCategories(subFilters);
        }
        ret.removeDuplicates();
        return ret;
    } break;
    case CategoryFilter::AppstreamIdWildcardFilter:
    case CategoryFilter::NotFilter:
    case CategoryFilter::PkgSectionFilter:
    case CategoryFilter::PkgWildcardFilter:
    case CategoryFilter::PkgNameFilter:
        break;
    }
    qCWarning(LIBDISCOVER_LOG) << "cannot infer categories from" << f.type;
    return {};
}

QStringList Category::involvedCategories() const
{
    return ::involvedCategories(m_filter);
}

bool CategoryFilter::operator==(const CategoryFilter &other) const
{
    if (other.type != type) {
        return false;
    }

    if (auto x = std::get_if<QString>(&value)) {
        return *x == std::get<QString>(other.value);
    } else {
        return std::get<QVector<CategoryFilter>>(value) == std::get<QVector<CategoryFilter>>(other.value);
    }
}
