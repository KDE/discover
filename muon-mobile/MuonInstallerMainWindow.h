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

#ifndef MUONINSTALLERDECLARATIVEVIEW_H
#define MUONINSTALLERDECLARATIVEVIEW_H

#include <LibQApt/Globals>
#include <MuonMainWindow.h>

class QDeclarativeView;
class ApplicationBackend;
namespace QApt { class Backend; }

class MuonInstallerMainWindow : public MuonMainWindow
{
    Q_OBJECT
    Q_PROPERTY(ApplicationBackend* appBackend READ appBackend NOTIFY appBackendChanged)
    Q_PROPERTY(QApt::Backend* backend READ backend NOTIFY appBackendChanged)
    public:
        explicit MuonInstallerMainWindow();

        ApplicationBackend* appBackend() const;
        QApt::Backend* backend() const;
        Q_SCRIPTABLE QAction* getAction(const QString& name);
        virtual QSize sizeHint() const;

    public slots:
        void setBackend(QApt::Backend* b);
        void openApplication(const QString& app);

    private slots:
        void triggerOpenApplication();

    signals:
        void appBackendChanged();
        void openApplicationInternal(const QString& appname);

    private:
        QString m_appToBeOpened;
        QDeclarativeView* m_view;
};

#endif // MUONINSTALLERDECLARATIVEVIEW_H
