/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "PKTransaction.h"
#include "PackageKitBackend.h"
#include "PackageKitResource.h"
#include <resources/AbstractResource.h>
#include <KMessageBox>
#include <KLocalizedString>
#include <PackageKit/packagekit-qt2/Transaction>
#include <QDebug>

PKTransaction::PKTransaction(AbstractResource* app, TransactionAction action, PackageKit::Transaction* pktrans)
    : Transaction(app, action)
    , m_trans(pktrans)
{
    m_trans->setParent(this);
    connect(pktrans, SIGNAL(finished(PackageKit::Transaction::Exit,uint)), SLOT(cleanup(PackageKit::Transaction::Exit,uint)));
    connect(pktrans, SIGNAL(errorCode(PackageKit::Transaction::Error,QString)), SLOT(errorFound(PackageKit::Transaction::Error,QString)));
    connect(pktrans, SIGNAL(mediaChangeRequired(PackageKit::Transaction::MediaType,QString,QString)),
            SLOT(mediaChange(PackageKit::Transaction::MediaType,QString,QString)));
    connect(pktrans, SIGNAL(requireRestart(PackageKit::Package::Restart,PackageKit::Package)),
            SLOT(requireRestard(PackageKit::Package::Restart,PackageKit::Package)));
}

void PKTransaction::cleanup(PackageKit::Transaction::Exit exit, uint runtime)
{
    qobject_cast<PackageKitBackend*>(resource()->backend())->removeTransaction(this);
    PackageKit::Transaction* t = new PackageKit::Transaction(resource());
    t->resolve(resource()->packageName(), PackageKit::Transaction::FilterNone);
    connect(t, SIGNAL(package(PackageKit::Package)), resource(), SLOT(updatePackage(PackageKit::Package)));
    connect(t, SIGNAL(package(PackageKit::Package)), t, SLOT(deleteLater()));
}

PackageKit::Transaction* PKTransaction::transaction()
{
    return m_trans;
}

void PKTransaction::errorFound(PackageKit::Transaction::Error err, const QString& error)
{
    KMessageBox::error(0, error, i18n("Found error %1", err));
}

void PKTransaction::mediaChange(PackageKit::Transaction::MediaType media, const QString& type, const QString& text)
{
    KMessageBox::information(0, text, i18n("Media Change requested: %1", type));
}

void PKTransaction::requireRestard(PackageKit::Package::Restart restart, const PackageKit::Package& p)
{
    KMessageBox::information(0, i18n("A change by '%1' suggests your system to be rebooted.", p.name()));
}
