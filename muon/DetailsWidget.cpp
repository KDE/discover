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
#include <QScrollArea>

// KDE
#include <KLocalizedString>

// LibQApt includes
#include <LibQApt/Backend>
#include <LibQApt/Package>

// Own includes
#include "DetailsTabs/MainTab.h"
#include "DetailsTabs/TechnicalDetailsTab.h"
#include "DetailsTabs/DependsTab.h"
#include "DetailsTabs/ChangelogTab.h"
#include "DetailsTabs/InstalledFilesTab.h"
#include "DetailsTabs/VersionTab.h"

DetailsWidget::DetailsWidget(QWidget *parent)
    : KTabWidget(parent)
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    setDocumentMode(true);

    DetailsTab *mainTab = new MainTab(this);
    m_detailsTabs.append(mainTab);
    m_detailsTabs.append(new TechnicalDetailsTab(this));
    m_detailsTabs.append(new DependsTab(this));
    m_detailsTabs.append(new InstalledFilesTab(0));
    m_detailsTabs.append(new VersionTab(0));
    m_detailsTabs.append(new ChangelogTab(this));

    // Hide until a package is clicked
    hide();

    connect(mainTab, SIGNAL(setInstall(QApt::Package*)),
            this, SIGNAL(setInstall(QApt::Package*)));
    connect(mainTab, SIGNAL(setRemove(QApt::Package*)),
            this, SIGNAL(setRemove(QApt::Package*)));
    connect(mainTab, SIGNAL(setUpgrade(QApt::Package*)),
            this, SIGNAL(setUpgrade(QApt::Package*)));
    connect(mainTab, SIGNAL(setReInstall(QApt::Package*)),
            this, SIGNAL(setReInstall(QApt::Package*)));
    connect(mainTab, SIGNAL(setKeep(QApt::Package*)),
            this, SIGNAL(setKeep(QApt::Package*)));
    connect(mainTab, SIGNAL(setPurge(QApt::Package*)),
            this, SIGNAL(setPurge(QApt::Package*)));
    connect(this, SIGNAL(emitHideButtonsSignal()),
	    mainTab, SLOT(hideButtons()));
}

DetailsWidget::~DetailsWidget()
{
}

void DetailsWidget::setBackend(QApt::Backend *backend)
{
    for (DetailsTab *tab : m_detailsTabs) {
        tab->setBackend(backend);
    }
}

void DetailsWidget::setPackage(QApt::Package *package)
{
    for (DetailsTab *tab : m_detailsTabs) {
        tab->setPackage(package);

        if (tab->shouldShow()) {
            addTab(tab, tab->name());
        } else {
            if (currentIndex() == indexOf(tab)) {
                setCurrentIndex(0);
            }
            removeTab(indexOf(tab));
        }
    }

    show();
}

void DetailsWidget::emitHideButtons()
{
  emit emitHideButtonsSignal();
}

void DetailsWidget::refreshTabs()
{
    for (DetailsTab *tab : m_detailsTabs) {
        tab->refresh();
    }
}

void DetailsWidget::clear()
{
    for (DetailsTab *tab : m_detailsTabs) {
        tab->clear();
    }

    hide();
}

#include "DetailsWidget.moc"
