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
#include <QtCore/QTextStream>
#include <QtGui/QLabel>
#include <QtGui/QScrollArea>
#include <QtGui/QTreeWidgetItem>
#include <QtGui/QVBoxLayout>

// KDE
#include <KDebug>
#include <KIO/Job>
#include <KJob>
#include <KLocale>
#include <KVBox>
#include <KTemporaryFile>
#include <KTextBrowser>
#include <KTreeWidgetSearchLineWidget>

// LibQApt includes
#include <libqapt/package.h>

// Own includes
#include "DetailsTabs/MainTab.h"

DetailsWidget::DetailsWidget(QWidget *parent)
    : KTabWidget(parent)
    , m_package(0)
    , m_changelogFile(0)
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

    // Main tab is in the Ui file. If anybody wants to write a C++ widget that
    // is equivalent to it, I would gladly use it. Layouting sucks with C++
    m_mainTab = new MainTab(this);

    // Technical tab
    m_technicalTab = new QScrollArea(this);

    // Dependencies tab
//     m_dependenciesTab = new QWidget;

    // File list tab
    m_filesTab = new KVBox;
    m_filesSearchEdit = new KTreeWidgetSearchLineWidget(m_filesTab);
    m_filesTreeWidget = new QTreeWidget(m_filesTab);
    m_filesTreeWidget->setHeaderLabel(i18nc("@title:tab", "Installed Files"));
    m_filesSearchEdit->searchLine()->setTreeWidget(m_filesTreeWidget);

    m_changelogTab = new KVBox;
    m_changelogBrowser = new KTextBrowser(m_changelogTab);


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

    if (package->isInstalled()) {
        addTab(m_filesTab, i18nc("@title:tab", "Installed Files"));
        populateFileList();
    } else {
        if (currentIndex() == indexOf(m_filesTab)) {
            setCurrentIndex(0); // Switch to the main tab
        }
        removeTab(indexOf(m_filesTab));
    }

    fetchChangelog();

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

void DetailsWidget::populateFileList()
{
    m_filesTreeWidget->clear();
    QStringList filesList = m_package->installedFilesList();

    foreach (const QString &file, filesList) {
        QStringList split = file.split(QChar('/'));
        QTreeWidgetItem *parentItem = 0;
        foreach (const QString &spl, split) {
            if (spl.isEmpty()) {
                continue;
            }
            if (parentItem) {
                bool there = false;
                int j = parentItem->childCount();
                for (int i = 0; i != j; i++) {
                    if (parentItem->child(i)->text(0) == spl) {
                        there = true;
                        parentItem = parentItem->child(i);
                        break;
                    }
                }
                if (!there) {
                    parentItem = new QTreeWidgetItem(parentItem, QStringList(spl));
                }
            } else {
                QList<QTreeWidgetItem*> list = m_filesTreeWidget->findItems(spl, Qt::MatchExactly);
                if (!list.isEmpty()) {
                    parentItem = list.first();
                } else {
                    parentItem = new QTreeWidgetItem(m_filesTreeWidget, QStringList(spl));
                }
            }
        }
    }
}

void DetailsWidget::changelogFetched(KJob *job)
{
    // Work around http://bugreports.qt.nokia.com/browse/QTBUG-2533 by forcibly resetting the CharFormat
    QTextCharFormat format;
    m_changelogBrowser->setCurrentCharFormat(format);
    QFile changelogFile(m_changelogFile->fileName());
    if (job->error() || !changelogFile.open(QFile::ReadOnly)) {
        m_changelogBrowser->setText(i18nc("@info/rich", "The list of changes is not available yet. "
                                          "Please use <link url='%1'>Launchpad</link> instead.",
                                          QString("http://launchpad.net/ubuntu/+source/" + m_package->sourcePackage())));
        return;
    }
    QTextStream stream(&changelogFile);
    m_changelogBrowser->setText(stream.readAll());
}

void DetailsWidget::fetchChangelog()
{
    m_changelogFile = new KTemporaryFile;
    m_changelogFile->setPrefix("muon");
    m_changelogFile->setSuffix(".txt");
    m_changelogFile->open();

    KIO::FileCopyJob *getJob = KIO::file_copy(m_package->changelogUrl(),
                                      m_changelogFile->fileName(), -1,
                                      KIO::Overwrite | KIO::HideProgressInfo);
    connect(getJob, SIGNAL(result(KJob*)),
            this, SLOT(changelogFetched(KJob*)));
}

#include "DetailsWidget.moc"
