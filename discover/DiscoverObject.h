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

#ifndef MUONDISCOVERMAINWINDOW_H
#define MUONDISCOVERMAINWINDOW_H

#include <QtCore/QUrl>

#include <QQuickView>

class AbstractResource;
class Category;
class QWindow;
class QQmlApplicationEngine;
class CachedNetworkAccessManagerFactory;
class ResourcesProxyModel;

class DiscoverObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(CompactMode compactMode READ compactMode WRITE setCompactMode NOTIFY compactModeChanged)
    Q_PROPERTY(bool isRoot READ isRoot CONSTANT)
    public:
        enum CompactMode { Auto, Compact, Full };
        Q_ENUM(CompactMode)

        explicit DiscoverObject(CompactMode mode);
        ~DiscoverObject() override;

        QStringList modes() const;
        void setupActions();

        CompactMode compactMode() const { return m_mode; }
        void setCompactMode(CompactMode mode);

        bool eventFilter(QObject * object, QEvent * event) override;

        Q_SCRIPTABLE QAction * action(const QString& name) const;
        Q_SCRIPTABLE static QString iconName(const QIcon& icon);

        void loadTest(const QUrl& url);

        static bool isRoot();
        QWindow* rootObject() const;
        void showPassiveNotification(const QString &msg);

    public Q_SLOTS:
        void openApplication(const QUrl& app);
        void openMimeType(const QString& mime);
        void openCategory(const QString& category);
        void openMode(const QString& mode);
        void openLocalPackage(const QUrl &localfile);

    private Q_SLOTS:
        void reportBug();
        void switchApplicationLanguage();
        void aboutApplication();

    Q_SIGNALS:
        void openSearch(const QString &search);
        void openApplicationInternal(AbstractResource* app);
        void listMimeInternal(const QString& mime);
        void listCategoryInternal(Category* cat);

        void compactModeChanged(CompactMode compactMode);
        void preventedClose();
        void unableToFind(const QString &resid);

    private:
        void setRootObjectProperty(const char *name, const QVariant &value);
        void integrateObject(QObject* object);
        QQmlApplicationEngine* engine() const { return m_engine; }

        QMap<QString, QAction*> m_collection;
        QQmlApplicationEngine * const m_engine;

        CompactMode m_mode;
        QScopedPointer<CachedNetworkAccessManagerFactory> m_networkAccessManagerFactory;
};

#endif // MUONINSTALLERDECLARATIVEVIEW_H
