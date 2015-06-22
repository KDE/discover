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

#include <KXmlGuiWindow>

class QQmlEngine;
class QLineEdit;
class QAptIntegration;
class AbstractResource;
class Category;
class QQuickWidget;

class MuonDiscoverMainWindow : public KXmlGuiWindow
{
    Q_OBJECT
    Q_PROPERTY(QObject* searchWidget READ searchWidget CONSTANT)
    public:
        explicit MuonDiscoverMainWindow();
        ~MuonDiscoverMainWindow();

        Q_SCRIPTABLE QAction* getAction(const QString& name);
        QSize sizeHint() const;
        
        void initialize();
        QStringList modes() const;
        void setupActions();
        QObject* searchWidget() const;

        void showEvent(QShowEvent*) Q_DECL_OVERRIDE;
        bool queryClose() Q_DECL_OVERRIDE;

    public slots:
        void openApplication(const QString& app);
        QUrl prioritaryFeaturedSource() const;
        QUrl featuredSource() const;
        void openMimeType(const QString& mime);
        void openCategory(const QString& category);
        void openMode(const QByteArray& mode);

    private slots:
        void triggerOpenApplication();

    signals:
        void openApplicationInternal(AbstractResource* app);
        void listMimeInternal(const QString& mime);
        void listCategoryInternal(const QString& name);

    private:
        void configureMenu();

        QString m_appToBeOpened;
        QLineEdit* m_searchText;
        QQuickWidget* m_view;
        QMenu* m_moreMenu;
        QMenu* m_advancedMenu;
};

#endif // MUONINSTALLERDECLARATIVEVIEW_H
