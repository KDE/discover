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

#include "DetailsWidget.h"

// Qt
#include <QtGui/QScrollArea>

// KDE
#include <KDebug>
#include <KLocale>

// LibQApt includes
#include <libqapt/package.h>

// Own includes
#include "DetailsTabs/MainTab.h"
#include "DetailsTabs/TechnicalDetailsTab.h"
#include "DetailsTabs/ChangelogTab.h"
#include "DetailsTabs/InstalledFilesTab.h"

DetailsWidget::DetailsWidget(QWidget *parent)
    : KTabWidget(parent)
    , m_package(0)
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

    m_mainTab = new MainTab(this);
    m_technicalTab = new TechnicalDetailsTab(this);
    //m_dependenciesTab = new QWidget;
    m_filesTab = new InstalledFilesTab;
    m_changelogTab = new ChangelogTab(this);


    addTab(m_mainTab, i18nc("@title:tab", "Details"));
    addTab(m_technicalTab, i18nc("@title:tab", "Technical Details"));
    // TODO: Needs serious work in LibQApt
    // addTab(m_dependenciesTab, i18nc("@title:tab", "Dependencies"));
    addTab(m_changelogTab, i18nc("@title:tab", "Changes List"));

    // Hide until a package is clicked
    hide();
}

DetailsWidget::~DetailsWidget()
{
}

void DetailsWidget::setPackage(QApt::Package *package)
{
    m_package = package;

    m_mainTab->setPackage(package);
    m_technicalTab->setPackage(package);
    m_changelogTab->setPackage(package);

    if (package->isInstalled()) {
        addTab(m_filesTab, i18nc("@title:tab", "Installed Files"));
        m_filesTab->setPackage(package);
    } else {
        if (currentIndex() == indexOf(m_filesTab)) {
            setCurrentIndex(0); // Switch to the main tab
        }
        removeTab(indexOf(m_filesTab));
    }

    show();
}

void DetailsWidget::refreshMainTabButtons()
{
    m_mainTab->refreshButtons();
}

void DetailsWidget::clear()
{
    m_mainTab->clear();
    m_package = 0;
    hide();
}

#include "DetailsWidget.moc"
