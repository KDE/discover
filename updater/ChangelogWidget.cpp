/***************************************************************************
 *   Copyright © 2011 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#include "ChangelogWidget.h"

// Qt includes
#include <QtCore/QParallelAnimationGroup>
#include <QtCore/QPropertyAnimation>
#include <QtCore/QStringBuilder>
#include <QtCore/QTextStream>
#include <QtGui/QToolButton>
#include <QtGui/QVBoxLayout>

// KDE includes
#include <KGlobal>
#include <KIO/Job>
#include <KJob>
#include <KLocale>
#include <KPixmapSequence>
#include <KPixmapSequenceOverlayPainter>
#include <KTemporaryFile>
#include <KTextBrowser>
#include <KDebug>

// LibQApt includes
#include <LibQApt/Backend>
#include <LibQApt/Changelog>

ChangelogWidget::ChangelogWidget(QWidget *parent)
        : QWidget(parent)
        , m_backend(0)
        , m_package(0)
        , m_show(false)
{
    QWidget *sideWidget = new QWidget(this);

    QToolButton *hideButton = new QToolButton(sideWidget);
    hideButton->setText(i18nc("@action:button", "Hide"));
    hideButton->setArrowType(Qt::DownArrow);
    hideButton->setAutoRaise(true);
    hideButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    connect(hideButton, SIGNAL(clicked()), this, SLOT(animatedHide()));

    QVBoxLayout *sideLayout = new QVBoxLayout(sideWidget);
    sideLayout->setMargin(0);
    sideLayout->setSpacing(0);
    sideLayout->addWidget(hideButton);
    sideLayout->addStretch();
    sideWidget->setLayout(sideLayout);

    m_changelogBrowser = new KTextBrowser(this);
    m_changelogBrowser->setFrameShape(QFrame::NoFrame);
    m_changelogBrowser->setFrameShadow(QFrame::Plain);
    m_changelogBrowser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *viewport = m_changelogBrowser->viewport();
    QPalette palette = viewport->palette();
    palette.setColor(viewport->backgroundRole(), Qt::transparent);
    palette.setColor(viewport->foregroundRole(), palette.color(QPalette::WindowText));
    viewport->setPalette(palette);

    m_busyWidget = new KPixmapSequenceOverlayPainter(this);
    m_busyWidget->setSequence(KPixmapSequence("process-working", KIconLoader::SizeSmallMedium));
    m_busyWidget->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_busyWidget->setWidget(this);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setMargin(0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(sideWidget);
    mainLayout->addWidget(m_changelogBrowser);

    int finalHeight = sizeHint().height();

    QPropertyAnimation *anim1 = new QPropertyAnimation(this, "maximumSize", this);
    anim1->setDuration(500);
    anim1->setEasingCurve(QEasingCurve::OutQuart);
    anim1->setStartValue(QSize(QWIDGETSIZE_MAX, 0));
    anim1->setEndValue(QSize(QWIDGETSIZE_MAX, finalHeight));
    QPropertyAnimation *anim2 = new QPropertyAnimation(this, "minimumSize", this);
    anim2->setDuration(500);
    anim2->setEasingCurve(QEasingCurve::OutQuart);
    anim2->setStartValue(QSize(QWIDGETSIZE_MAX, 0));
    anim2->setEndValue(QSize(QWIDGETSIZE_MAX, finalHeight));

    m_expandWidget = new QParallelAnimationGroup(this);
    m_expandWidget->addAnimation(anim1);
    m_expandWidget->addAnimation(anim2);
}

void ChangelogWidget::setBackend(QApt::Backend *backend)
{
    m_backend = backend;
}

void ChangelogWidget::setPackage(QApt::Package *package)
{
    m_package = package;

    package ? fetchChangelog() : animatedHide();
}

void ChangelogWidget::show()
{
    QWidget::show();

    if (!m_show) {
        m_show = true;
        disconnect(m_expandWidget, SIGNAL(finished()), this, SLOT(hide()));
        m_expandWidget->setDirection(QAbstractAnimation::Forward);
        m_expandWidget->start();
    }
}

void ChangelogWidget::animatedHide()
{
    m_show = false;
    m_changelogBrowser->clear();

    m_expandWidget->setDirection(QAbstractAnimation::Backward);
    m_expandWidget->start();
    connect(m_expandWidget, SIGNAL(finished()), this, SLOT(hide()));
}

void ChangelogWidget::stopPendingJobs()
{
    auto iter = m_jobHash.constBegin();
    while (iter != m_jobHash.constEnd()) {
        KJob *getJob = iter.key();
        disconnect(getJob, SIGNAL(result(KJob*)),
                   this, SLOT(changelogFetched(KJob*)));
        iter++;
    }

    m_jobHash.clear();
}

void ChangelogWidget::changelogFetched(KJob *job)
{
    if (!m_package) {
        m_jobHash.remove(job);
        return;
    }

    // Work around http://bugreports.qt.nokia.com/browse/QTBUG-2533 by forcibly resetting the CharFormat
    QTextCharFormat format;
    m_changelogBrowser->setCurrentCharFormat(format);
    QFile changelogFile(m_jobHash[job]);
    m_jobHash.remove(job);

    if (job->error() || !changelogFile.open(QFile::ReadOnly)) {
        if (m_package->origin() == QLatin1String("Ubuntu")) {
            m_changelogBrowser->setText(i18nc("@info/rich", "The list of changes is not yet available. "
                                            "Please use <link url='%1'>Launchpad</link> instead.",
                                            QString("http://launchpad.net/ubuntu/+source/" + m_package->sourcePackage())));
        } else {
            m_changelogBrowser->setText(i18nc("@info", "The list of changes is not yet available."));
        }
    }
    else {
        QTextStream stream(&changelogFile);
        const QApt::Changelog log(stream.readAll(), m_package->sourcePackage());
        QString description = buildDescription(log);

        m_changelogBrowser->setHtml(description);
    }

    m_busyWidget->stop();
    if (!m_show) {
        animatedHide();
    }

    changelogFile.remove();
}

void ChangelogWidget::fetchChangelog()
{
    show();
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

    m_jobHash[getJob] = filename;
    connect(getJob, SIGNAL(result(KJob*)),
            this, SLOT(changelogFetched(KJob*)));
}

QString ChangelogWidget::buildDescription(const QApt::Changelog &changelog)
{
    QString description;

    QApt::ChangelogEntryList entries = changelog.newEntriesSince(m_package->installedVersion());

    if (entries.size() < 1) {
        return description;
    }

    foreach(const QApt::ChangelogEntry &entry, entries) {
        description += i18nc("@info:label Refers to a software version, Ex: Version 1.2.1:",
                             "Version %1:", entry.version());

        QString issueDate = KGlobal::locale()->formatDateTime(entry.issueDateTime(), KLocale::ShortDate);
        description += QLatin1String("<p>") %
                       i18nc("@info:label", "This update was issued on %1", issueDate) %
                       QLatin1String("</p>");

        QString updateText = entry.description();
        updateText.replace('\n', QLatin1String("<br/>"));
        description += QLatin1String("<p><pre>") %
                       updateText %
                       QLatin1String("</pre></p>");
    }

    return description;
}
