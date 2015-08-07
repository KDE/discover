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
    Q_PROPERTY(QUrl prioritaryFeaturedSource READ prioritaryFeaturedSource CONSTANT)
    Q_PROPERTY(QUrl featuredSource READ featuredSource CONSTANT)
    Q_PROPERTY(bool isCompact READ isCompact NOTIFY compactChanged)
    Q_PROPERTY(qreal actualWidth READ actualWidth NOTIFY actualWidthChanged)
    public:
        explicit MuonDiscoverMainWindow();
        ~MuonDiscoverMainWindow();

        QSize sizeHint() const override;
        
        void initialize();
        QStringList modes() const;
        void setupActions();

        bool queryClose() Q_DECL_OVERRIDE;

        QUrl prioritaryFeaturedSource() const;
        QUrl featuredSource() const;

        bool isCompact() const;
        qreal actualWidth() const;

        void resizeEvent(QResizeEvent * event);
        void showEvent(QShowEvent * event);

    public slots:
        void openApplication(const QString& app);
        void openMimeType(const QString& mime);
        void openCategory(const QString& category);
        void openMode(const QByteArray& mode);
        void showMenu(int x, int y);

    private slots:
        void triggerOpenApplication();

    signals:
        void openApplicationInternal(AbstractResource* app);
        void listMimeInternal(const QString& mime);
        void listCategoryInternal(const QString& name);

        void compactChanged(bool isCompact);
        void actualWidthChanged(qreal actualWidth);

    private:
        void configureMenu();

        QString m_appToBeOpened;
        QQuickWidget* m_view;
        QMenu* m_moreMenu;
        QMenu* m_advancedMenu;
};

#endif // MUONINSTALLERDECLARATIVEVIEW_H
