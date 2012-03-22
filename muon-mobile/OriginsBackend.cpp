/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "OriginsBackend.h"
#include "BackendsSingleton.h"
#include "MuonInstallerMainWindow.h"
#include <ApplicationBackend.h>
#include <QProcess>
#include <QDebug>
#include <LibQApt/Backend>
#include <KMessageBox>
#include <KLocalizedString>

OriginsBackend::OriginsBackend(QObject* parent)
    : QObject(parent)
{
    connect(BackendsSingleton::self()->applicationBackend(), SIGNAL(reloadFinished()), SIGNAL(originsChanged()));
}

void OriginsBackend::addRepository(const QString& repository)
{
    QProcess* p = new QProcess(this);
    p->setProcessChannelMode(QProcess::MergedChannels);
    connect(p, SIGNAL(finished(int)), SLOT(additionDone(int)));
    connect(p, SIGNAL(finished(int)), p, SLOT(deleteLater()));
    p->start("kdesudo", QStringList("--") << "apt-add-repository" << "-y" << repository);
}

void OriginsBackend::removeRepository(const QString& repository)
{
    QProcess* p = new QProcess(this);
    p->setProcessChannelMode(QProcess::MergedChannels);
    connect(p, SIGNAL(finished(int)), SLOT(removalDone(int)));
    connect(p, SIGNAL(finished(int)), p, SLOT(deleteLater()));
    p->start("kdesudo", QStringList("--") << "apt-add-repository" << "--remove" << "-y" << repository);
}

QStringList OriginsBackend::labels() const
{
    return BackendsSingleton::self()->backend()->originLabels();
}

QString OriginsBackend::labelsOrigin(const QString& label) const
{
    return BackendsSingleton::self()->backend()->origin(label);
}

void OriginsBackend::additionDone(int processErrorCode)
{
    if(processErrorCode==0) {
        BackendsSingleton::self()->applicationBackend(), SLOT(reload());
    } else {
        QProcess* p = qobject_cast<QProcess*>(sender());
        Q_ASSERT(p);
        QByteArray errorMessage = p->readAllStandardOutput();
        KMessageBox::error(BackendsSingleton::self()->mainWindow(), errorMessage, i18n("Adding Origins..."));
    }
}

void OriginsBackend::removalDone(int processErrorCode)
{
    if(processErrorCode==0) {
        BackendsSingleton::self()->applicationBackend(), SLOT(reload());
    } else {
        QProcess* p = qobject_cast<QProcess*>(sender());
        Q_ASSERT(p);
        QByteArray errorMessage = p->readAllStandardOutput();
        KMessageBox::error(BackendsSingleton::self()->mainWindow(), errorMessage, i18n("Removing Origins..."));
    }
}
