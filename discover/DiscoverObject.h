/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef DISCOVEROBJECT_H
#define DISCOVEROBJECT_H

#include <QUrl>

#include <QQuickView>

class AbstractResource;
class Category;
class QWindow;
class QQmlApplicationEngine;
class CachedNetworkAccessManagerFactory;

class DiscoverObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(CompactMode compactMode READ compactMode WRITE setCompactMode NOTIFY compactModeChanged)
    Q_PROPERTY(bool isRoot READ isRoot CONSTANT)
    Q_PROPERTY(QRect initialGeometry READ initialGeometry CONSTANT)

    public:
        enum CompactMode { Auto, Compact, Full };
        Q_ENUM(CompactMode)

        explicit DiscoverObject(CompactMode mode, const QVariantMap &initialProperties);
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
        QQuickWindow* rootObject() const;
        void showPassiveNotification(const QString &msg);
        QRect initialGeometry() const;

        QString describeSources() const;

    public Q_SLOTS:
        void openApplication(const QUrl& app);
        void openMimeType(const QString& mime);
        void openCategory(const QString& category);
        void openMode(const QString& mode);
        void openLocalPackage(const QUrl &localfile);

        void reboot();

    private Q_SLOTS:
        void reportBug();
        void switchApplicationLanguage();
        void aboutApplication();

    Q_SIGNALS:
        void openSearch(const QString &search);
        void openApplicationInternal(AbstractResource* app);
        void listMimeInternal(const QString& mime);
        void listCategoryInternal(Category* cat);

        void compactModeChanged(DiscoverObject::CompactMode compactMode);
        void preventedClose();
        void unableToFind(const QString &resid);
        void openErrorPage(const QString &errorMessage);

    private:
        void showLoadingPage();
        void integrateObject(QObject* object);
        QQmlApplicationEngine* engine() const { return m_engine; }

        QMap<QString, QAction*> m_collection;
        QQmlApplicationEngine * const m_engine;

        CompactMode m_mode;
        QScopedPointer<CachedNetworkAccessManagerFactory> m_networkAccessManagerFactory;
};

#endif // DISCOVEROBJECT_H
