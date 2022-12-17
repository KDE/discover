/*
 *   SPDX-FileCopyrightText: 2010 Jonathan Thomas <echidnaman@kubuntu.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>
#include <QPair>
#include <QSet>
#include <QString>
#include <QUrl>
#include <QVector>
#include <variant>

#include "discovercommon_export.h"

class QXmlStreamReader;
class QTimer;

class CategoryFilter
{
    Q_GADGET
public:
    enum FilterType {
        CategoryNameFilter,
        PkgSectionFilter,
        PkgWildcardFilter,
        PkgNameFilter,
        AppstreamIdWildcardFilter,
        OrFilter,
        AndFilter,
        NotFilter,
    };
    Q_ENUM(FilterType)

    FilterType type;
    std::variant<QString, QVector<CategoryFilter>> value;

    bool operator==(const CategoryFilter &other) const;
    bool operator!=(const CategoryFilter &other) const
    {
        return !operator==(other);
    }
};

class DISCOVERCOMMON_EXPORT Category : public QObject
{
    Q_OBJECT
public:
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString icon READ icon CONSTANT)
    Q_PROPERTY(QObject *parent READ parent CONSTANT)
    Q_PROPERTY(QVariantList subcategories READ subCategoriesVariant NOTIFY subCategoriesChanged)
    explicit Category(QSet<QString> pluginNames, QObject *parent = nullptr);

    Category(const QString &name,
             const QString &iconName,
             const CategoryFilter &filters,
             const QSet<QString> &pluginName,
             const QVector<Category *> &subCategories,
             bool isAddons);
    ~Category() override;

    QString name() const;
    // You should never attempt to change the name of anything that is not a leaf category
    // as the results could be potentially detremental to the function of the category filters
    void setName(const QString &name);
    QString icon() const;
    void setFilter(const CategoryFilter &filter);
    CategoryFilter filter() const;
    QVector<Category *> subCategories() const;
    QVariantList subCategoriesVariant() const;

    static void sortCategories(QVector<Category *> &cats);
    static void addSubcategory(QVector<Category *> &cats, Category *cat);
    /**
     * Add a subcategory to this category. This function should only
     * be used during the initialisation stage, before adding the local
     * root category to the global root category model.
     */
    void addSubcategory(Category *cat);
    void parseData(const QString &path, QXmlStreamReader *xml);
    bool blacklistPlugins(const QSet<QString> &pluginName);
    bool isAddons() const
    {
        return m_isAddons;
    }
    qint8 priority() const
    {
        return m_priority;
    }
    bool matchesCategoryName(const QString &name) const;

    Q_SCRIPTABLE bool contains(Category *cat) const;
    Q_SCRIPTABLE bool contains(const QVariantList &cats) const;

    static bool categoryLessThan(Category *c1, const Category *c2);
    static bool blacklistPluginsInVector(const QSet<QString> &pluginNames, QVector<Category *> &subCategories);

    QStringList involvedCategories() const;
    QString untranslatedName() const
    {
        return m_untranslatedName;
    }

Q_SIGNALS:
    void subCategoriesChanged();
    void nameChanged();

private:
    QString m_name;
    QString m_untranslatedName;
    QString m_iconString;
    CategoryFilter m_filter;
    QVector<Category *> m_subCategories;

    CategoryFilter parseIncludes(QXmlStreamReader *xml);
    QSet<QString> m_plugins;
    bool m_isAddons = false;
    qint8 m_priority = 0;
    QTimer *m_subCategoriesChanged;
};
