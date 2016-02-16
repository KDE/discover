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
#include <KActionCollection>

class QQmlEngine;
class QLineEdit;
class QAptIntegration;
class AbstractResource;
class Category;
class QQuickWidget;
class QMenu;

class MuonDiscoverMainWindow : public QQuickView
{
    Q_OBJECT
    Q_PROPERTY(QUrl prioritaryFeaturedSource READ prioritaryFeaturedSource CONSTANT)
    Q_PROPERTY(QUrl featuredSource READ featuredSource CONSTANT)
    Q_PROPERTY(bool isCompact READ isCompact NOTIFY compactChanged)
    Q_PROPERTY(qreal actualWidth READ actualWidth NOTIFY actualWidthChanged)
    Q_PROPERTY(CompactMode compactMode READ compactMode WRITE setCompactMode)
    public:
        enum CompactMode { Auto, Compact, Full };
        Q_ENUMS(CompactMode)

        explicit MuonDiscoverMainWindow(CompactMode mode);
        ~MuonDiscoverMainWindow();

        void initialize();
        QStringList modes() const;
        void setupActions();

        void closeEvent(QCloseEvent *e) /*override*/;
        bool queryClose();

        QUrl prioritaryFeaturedSource() const;
        QUrl featuredSource() const;

        bool isCompact() const;
        qreal actualWidth() const;

        CompactMode compactMode() const { return m_mode; }
        void setCompactMode(CompactMode mode);

        void resizeEvent(QResizeEvent * event) override;
        void showEvent(QShowEvent * event) override;

        Q_SCRIPTABLE QAction * action(const QString& name);
        Q_SCRIPTABLE QString iconName(const QIcon& icon);

    public Q_SLOTS:
        void openApplication(const QString& app);
        void openMimeType(const QString& mime);
        void openCategory(const QString& category);
        void openMode(const QByteArray& mode);

    private Q_SLOTS:
        void triggerOpenApplication();
        void appHelpActivated();
        void reportBug();
        void switchApplicationLanguage();
        void aboutApplication();
        void configureShortcuts();

    Q_SIGNALS:
        void openApplicationInternal(AbstractResource* app);
        void listMimeInternal(const QString& mime);
        void listCategoryInternal(const QString& name);

        void compactChanged(bool isCompact);
        void actualWidthChanged(qreal actualWidth);
        void preventedClose();

    private:
        void configureSources();
        void configureMenu();

        KActionCollection* actionCollection() { return &m_collection; }

        QString m_appToBeOpened;
        KActionCollection m_collection;

        CompactMode m_mode;
};

#endif // MUONINSTALLERDECLARATIVEVIEW_H
