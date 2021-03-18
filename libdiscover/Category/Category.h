/*
 *   SPDX-FileCopyrightText: 2010 Jonathan Thomas <echidnaman@kubuntu.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef CATEGORY_H
#define CATEGORY_H

#include <QObject>
#include <QPair>
#include <QSet>
#include <QUrl>
#include <QVector>

#include "discovercommon_export.h"

class QDomNode;

enum FilterType {
    InvalidFilter,
    CategoryFilter,
    PkgSectionFilter,
    PkgWildcardFilter,
    PkgNameFilter,
    AppstreamIdWildcardFilter,
};

class DISCOVERCOMMON_EXPORT Category : public QObject
{
    Q_OBJECT
public:
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString icon READ icon CONSTANT)
    Q_PROPERTY(QObject *parent READ parent CONSTANT)
    Q_PROPERTY(QUrl decoration READ decoration CONSTANT)
    Q_PROPERTY(QVariantList subcategories READ subCategoriesVariant NOTIFY subCategoriesChanged)
    explicit Category(QSet<QString> pluginNames, QObject *parent = nullptr);

    Category(const QString &name,
             const QString &iconName,
             const QVector<QPair<FilterType, QString>> &orFilters,
             const QSet<QString> &pluginName,
             const QVector<Category *> &subCategories,
             const QUrl &decoration,
             bool isAddons);
    ~Category() override;

    QString name() const;
    // You should never attempt to change the name of anything that is not a leaf category
    // as the results could be potentially detremental to the function of the category filters
    void setName(const QString &name);
    QString icon() const;
    QVector<QPair<FilterType, QString>> andFilters() const;
    void setAndFilter(QVector<QPair<FilterType, QString>> filters);
    QVector<QPair<FilterType, QString>> orFilters() const;
    QVector<QPair<FilterType, QString>> notFilters() const;
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
    void parseData(const QString &path, const QDomNode &data);
    bool blacklistPlugins(const QSet<QString> &pluginName);
    bool isAddons() const
    {
        return m_isAddons;
    }
    QUrl decoration() const;
    bool matchesCategoryName(const QString &name) const;

    Q_SCRIPTABLE bool contains(Category *cat) const;
    Q_SCRIPTABLE bool contains(const QVariantList &cats) const;

    static bool categoryLessThan(Category *c1, const Category *c2);
    static bool blacklistPluginsInVector(const QSet<QString> &pluginNames, QVector<Category *> &subCategories);

Q_SIGNALS:
    void subCategoriesChanged();
    void nameChanged();

private:
    QString m_name;
    QString m_iconString;
    QUrl m_decoration;
    QVector<QPair<FilterType, QString>> m_andFilters;
    QVector<QPair<FilterType, QString>> m_orFilters;
    QVector<QPair<FilterType, QString>> m_notFilters;
    QVector<Category *> m_subCategories;

    QVector<QPair<FilterType, QString>> parseIncludes(const QDomNode &data);
    QSet<QString> m_plugins;
    bool m_isAddons = false;
};

#endif
