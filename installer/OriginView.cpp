/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#include "OriginView.h"

// KDE includes
#include <KIcon>
#include <KLocale>
#include <KDebug>

// LibQApt includes
#include <LibQApt/Backend>

OriginView::OriginView(QWidget *parent)
    : QTreeView(parent)
    , m_backend(0)
{
    setHeaderHidden(true);
    setUniformRowHeights(true);
    setIconSize(QSize(24,24));

    m_originModel = new QStandardItemModel;
}

OriginView::~OriginView()
{
}

void OriginView::setBackend(QApt::Backend *backend)
{
    m_backend = backend;

    populateOrigins();
    setModel(m_originModel);
}

void OriginView::populateOrigins()
{
    QStringList originLabels = m_backend->originLabels();
    QStringList originNames;
    foreach (const QString &originLabel, originLabels) {
        originNames << m_backend->origin(originLabel);
    }

    if (originNames.contains("Ubuntu")) {
        int index = originNames.indexOf("Ubuntu");
        originNames.move(index, 0); // Move to front of the list
    }

    if (originNames.contains("Canonical")) {
        int index = originNames.indexOf("Canonical");
        originNames.move(index, 1); // Move to 2nd spot
    }

    QStandardItem *parentItem = m_originModel->invisibleRootItem();

    QStandardItem *availableItem = new QStandardItem;
    availableItem->setEditable(false);
    availableItem->setIcon(KIcon("applications-other").pixmap(32,32));
    availableItem->setText(i18nc("@item:inlistbox Parent item for available software", "Get Software"));
    parentItem->appendRow(availableItem);

    QStandardItem *installedItem = new QStandardItem;
    installedItem->setEditable(false);
    installedItem->setIcon(KIcon("computer"));
    installedItem->setText(i18nc("@item:inlistbox Parent item for installed software", "Installed Software"));
    parentItem->appendRow(installedItem);

    parentItem = availableItem;
    foreach(const QString & originName, originNames) {
        QString originLabel = m_backend->originLabel(originName);
        QStandardItem *originItem = new QStandardItem;
        originItem->setEditable(false);
        originItem->setText(originLabel);

        if (originName == "Ubuntu") {
            originItem->setText(i18n("Provided by Kubuntu"));
            originItem->setIcon(KIcon("ubuntu-logo"));
        }

        if (originName == "Canonical") {
            originItem->setText(i18n("Canonical Partners"));
            originItem->setIcon(KIcon("partner"));
        }

        if (originName.startsWith(QLatin1String("LP-PPA"))) {
            originItem->setIcon(KIcon("user-identity"));
        }

        availableItem->appendRow(originItem);
    }

    parentItem = installedItem;
    foreach(const QString & originName, originNames) {
        QString originLabel = m_backend->originLabel(originName);
        QStandardItem *originItem = new QStandardItem;
        originItem->setEditable(false);
        originItem->setText(originLabel);

        if (originName == "Ubuntu") {
            originItem->setText("Kubuntu");
            originItem->setIcon(KIcon("ubuntu-logo"));
        }

        if (originName == "Canonical") {
            originItem->setIcon(KIcon("partner"));
        }

        if (originName.startsWith(QLatin1String("LP-PPA"))) {
            originItem->setIcon(KIcon("user-identity"));
        }

        installedItem->appendRow(originItem);
    }
}

#include "OriginView.moc"
