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

#include "ChangelogTab.h"

// Qt includes
#include <QtCore/QTextStream>

// KDE includes
#include <KDialog>
#include <KIO/Job>
#include <KJob>
#include <KLocale>
#include <KPixmapSequence>
#include <KPixmapSequenceOverlayPainter>
#include <KTemporaryFile>
#include <KTextBrowser>

// LibQApt includes
#include <LibQApt/Package>
#include <LibQApt/Changelog>

ChangelogTab::ChangelogTab(QWidget *parent)
    : KVBox(parent)
    , m_package(0)
{
    m_changelogBrowser = new KTextBrowser(this);

    m_busyWidget = new KPixmapSequenceOverlayPainter(this);
    m_busyWidget->setSequence(KPixmapSequence("process-working", KIconLoader::SizeSmallMedium));
    m_busyWidget->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_busyWidget->setWidget(m_changelogBrowser->viewport());
}

ChangelogTab::~ChangelogTab()
{
}

void ChangelogTab::setPackage(QApt::Package *package)
{
    // Clean up old jobs
    QHash<KJob *, QString>::const_iterator i = m_jobFilenames.constBegin();
    while (i != m_jobFilenames.constEnd()) {
        i.key()->kill();
        ++i;
    }
    m_jobFilenames.clear(); // We don't delete the KJob pointers, they delete themselves

    m_package = package;
    fetchChangelog();
}

void ChangelogTab::changelogFetched(KJob *job)
{
    // Work around http://bugreports.qt.nokia.com/browse/QTBUG-2533 by forcibly resetting the CharFormat
    QTextCharFormat format;
    m_changelogBrowser->setCurrentCharFormat(format);
    QFile changelogFile(m_jobFilenames[job]);
    m_busyWidget->stop();
    if (job->error() || !changelogFile.open(QFile::ReadOnly)) {
        m_changelogBrowser->setText(i18nc("@info/rich", "The list of changes is not available yet. "
                                          "Please use <link url='%1'>Launchpad</link> instead.",
                                          QString("http://launchpad.net/ubuntu/+source/" + m_package->sourcePackage())));
    }
    else {
        QTextStream stream(&changelogFile);
        const QApt::Changelog log(stream.readAll(), m_package->sourcePackage());
        m_changelogBrowser->setText(log.text());
    }

    m_jobFilenames.remove(job);
    changelogFile.remove();
}

void ChangelogTab::fetchChangelog()
{
    m_changelogBrowser->clear();
    m_busyWidget->start();

    KTemporaryFile *changelogFile = new KTemporaryFile;
    changelogFile->setAutoRemove(false);
    changelogFile->setPrefix("muon");
    changelogFile->setSuffix(".txt");
    changelogFile->open();
    QString filename = changelogFile->fileName();
    delete changelogFile;

    KIO::FileCopyJob *getJob = KIO::file_copy(m_package->changelogUrl(),
                               filename, -1,
                               KIO::Overwrite | KIO::HideProgressInfo);
    m_jobFilenames.insert(getJob, filename);
    connect(getJob, SIGNAL(result(KJob *)),
            this, SLOT(changelogFetched(KJob *)));
}

#include "ChangelogTab.moc"
