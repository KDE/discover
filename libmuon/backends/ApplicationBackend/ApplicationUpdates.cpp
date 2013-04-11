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

#include "ApplicationUpdates.h"

// Qt includes
#include <QIcon>
#include <QAction>

// KDE includes
#include <KProtocolManager>
#include <KMessageBox>
#include <KActionCollection>
#include <KDebug>

// Own includes
#include <LibQApt/Transaction>

#include <MuonMainWindow.h>
#include <ChangesDialog.h>
#include <MuonStrings.h>
#include <QAptActions.h>

#include "Application.h"
#include "ApplicationBackend.h"

ApplicationUpdates::ApplicationUpdates(ApplicationBackend* parent)
    : AbstractBackendUpdater(parent)
    , m_aptBackend(nullptr)
    , m_appBackend(parent)
    , m_lastRealProgress(0)
    , m_eta(0)
    , m_progressing(false)
{
    connect(m_appBackend, SIGNAL(reloadFinished()),
            this, SLOT(calculateUpdates()));
}

bool ApplicationUpdates::hasUpdates() const
{
    return m_appBackend->updatesCount()>0;
}

qreal ApplicationUpdates::progress() const
{
    return m_lastRealProgress;
}

long unsigned int ApplicationUpdates::remainingTime() const
{
    return m_eta;
}

void ApplicationUpdates::setBackend(QApt::Backend* backend)
{
    Q_ASSERT(!m_aptBackend || m_aptBackend==backend);
    m_aptBackend = backend;
}

QList<AbstractResource*> ApplicationUpdates::toUpdate() const
{
    return m_toUpdate;
}

void ApplicationUpdates::prepare()
{
    m_aptBackend->markPackages(m_aptBackend->markedPackages(), QApt::Package::ToKeep);
    m_updatesCache = m_aptBackend->currentCacheState();
    m_aptBackend->markPackagesForDistUpgrade();
    calculateUpdates();
}

void ApplicationUpdates::start()
{
    auto changes = m_aptBackend->stateChanges(m_updatesCache, QApt::PackageList());
    if(changes.isEmpty()) {
        kWarning() << "couldn't find any apt updates";
        setProgressing(false);
        return;
    }
    for(auto it=changes.begin(); it!=changes.end(); ) {
        if(it.key()&QApt::Package::ToUpgrade) {
            it = changes.erase(it);
        } else
            ++it;
    }
    
    // Confirm additional changes beyond upgrading the files
    if(!changes.isEmpty()) {
        ChangesDialog d(m_appBackend->mainWindow(), changes);
        if(d.exec()==QDialog::Rejected) {
            setProgressing(false);
            return;
        }
    }

    // Create and run the transaction
    setupTransaction(m_aptBackend->commitChanges());
}

void ApplicationUpdates::addResources(const QList<AbstractResource*>& apps)
{
    QList<QApt::Package*> packages;
    foreach(AbstractResource* res, apps) {
        Application* app = qobject_cast<Application*>(res);
        Q_ASSERT(app);
        packages += app->package();
    }
    m_aptBackend->markPackages(packages, QApt::Package::ToInstall);
}

void ApplicationUpdates::removeResources(const QList<AbstractResource*>& apps)
{
    QList<QApt::Package*> packages;
    foreach(AbstractResource* res, apps) {
        Application* app = qobject_cast<Application*>(res);
        Q_ASSERT(app);
        packages += app->package();
    }
    m_aptBackend->markPackages(packages, QApt::Package::ToKeep);
}

void ApplicationUpdates::setProgress(int progress)
{
    if (progress > 100)
        return;

    if (progress > m_lastRealProgress || progress<0) {
        m_lastRealProgress = progress;
        emit progressChanged((qreal)progress);
    }
}

void ApplicationUpdates::etaChanged(quint64 eta)
{
    if(m_eta != eta) {
        m_eta = eta;
        emit remainingTimeChanged();
    }
}

void ApplicationUpdates::installMessage(const QString& msg)
{
    setStatusMessage(msg);
}

void ApplicationUpdates::errorOccurred(QApt::ErrorCode error)
{
    if(error!=QApt::Success) {
        QAptActions::self()->displayTransactionError(error, m_trans);
        setProgressing(false);
    }
}

void ApplicationUpdates::setupTransaction(QApt::Transaction *trans)
{
    Q_ASSERT(trans);
    // Provide proxy/locale to the transaction
    if (KProtocolManager::proxyType() == KProtocolManager::ManualProxy) {
        trans->setProxy(KProtocolManager::proxyFor("http"));
    }

    trans->setLocale(QLatin1String(setlocale(LC_MESSAGES, 0)));

    connect(trans, SIGNAL(errorOccurred(QApt::ErrorCode)),
            SLOT(errorOccurred(QApt::ErrorCode)));
    connect(trans, SIGNAL(progressChanged(int)), SLOT(setProgress(int)));
    connect(trans, SIGNAL(statusDetailsChanged(QString)), SLOT(installMessage(QString)));
    connect(trans, SIGNAL(cancellableChanged(bool)), SIGNAL(cancelableChanged(bool)));
    connect(trans, SIGNAL(finished(QApt::ExitStatus)), trans, SLOT(deleteLater()));
    connect(trans, SIGNAL(finished(QApt::ExitStatus)), SLOT(transactionFinished(QApt::ExitStatus)));
    connect(trans, SIGNAL(statusChanged(QApt::TransactionStatus)),
            this, SLOT(statusChanged(QApt::TransactionStatus)));
    connect(trans, SIGNAL(mediumRequired(QString,QString)),
            this, SLOT(provideMedium(QString,QString)));
    connect(trans, SIGNAL(promptUntrusted(QStringList)),
            this, SLOT(untrustedPrompt(QStringList)));
    connect(trans, SIGNAL(downloadSpeedChanged(quint64)),
            this, SIGNAL(downloadSpeedChanged(quint64)));
    trans->run();
    m_trans = trans;
    setProgressing(true);
}

bool ApplicationUpdates::isAllMarked() const
{
    QApt::PackageList upgradeable = m_aptBackend->upgradeablePackages();
    int markedCount = m_aptBackend->packageCount(QApt::Package::ToUpgrade);
    return markedCount >= upgradeable.count();
}

QDateTime ApplicationUpdates::lastUpdate() const
{
    return m_aptBackend->timeCacheLastUpdated();
}

bool ApplicationUpdates::isCancelable() const
{
    return m_trans && m_trans->isCancellable();
}

bool ApplicationUpdates::isProgressing() const
{
    return m_progressing;
}

void ApplicationUpdates::provideMedium(const QString &label, const QString &medium)
{
    QString title = i18nc("@title:window", "Media Change Required");
    QString text = i18nc("@label", "Please insert %1 into <filename>%2</filename>",
                         label, medium);

    KMessageBox::information(QAptActions::self()->mainWindow(), text, title);
    m_trans->provideMedium(medium);
}

void ApplicationUpdates::untrustedPrompt(const QStringList &untrustedPackages)
{
    QString title = i18nc("@title:window", "Warning - Unverified Software");
    QString text = i18ncp("@label",
                          "The following piece of software cannot be verified. "
                          "<warning>Installing unverified software represents a "
                          "security risk, as the presence of unverifiable software "
                          "can be a sign of tampering.</warning> Do you wish to continue?",
                          "The following pieces of software cannot be verified. "
                          "<warning>Installing unverified software represents a "
                          "security risk, as the presence of unverifiable software "
                          "can be a sign of tampering.</warning> Do you wish to continue?",
                          untrustedPackages.size());
    int result = KMessageBox::warningContinueCancelList(QAptActions::self()->mainWindow(), 
                                                        text, untrustedPackages, title);

    bool installUntrusted = (result == KMessageBox::Continue);
    m_trans->replyUntrustedPrompt(installUntrusted);
}

void ApplicationUpdates::configFileConflict(const QString &currentPath, const QString &newPath)
{
    QString title = i18nc("@title:window", "Configuration File Changed");
    QString text = i18nc("@label Notifies a config file change",
                         "A new version of the configuration file "
                         "<filename>%1</filename> is available, but your version has "
                         "been modified. Would you like to keep your current version "
                         "or install the new version?", currentPath);

    KGuiItem useNew(i18nc("@action Use the new config file", "Use New Version"));
    KGuiItem useOld(i18nc("@action Keep the old config file", "Keep Old Version"));

    // TODO: diff current and new paths
    Q_UNUSED(newPath)

    int ret = KMessageBox::questionYesNo(QAptActions::self()->mainWindow(), text, title, useNew, useOld);

    m_trans->resolveConfigFileConflict(currentPath, (ret == KMessageBox::Yes));
}

void ApplicationUpdates::statusChanged(QApt::TransactionStatus status)
{
    switch (status) {
        case QApt::SetupStatus:
            setProgressing(true);
            setStatusMessage(i18nc("@info Status info, widget title",
                                        "Starting"));
            setProgress(-1);
            break;
        case QApt::AuthenticationStatus:
            setStatusMessage(i18nc("@info Status info, widget title",
                                        "Waiting for Authentication"));
            setProgress(-1);
            break;
        case QApt::WaitingStatus:
            setStatusMessage(i18nc("@info Status information, widget title",
                                        "Waiting"));
            setStatusDetail(i18nc("@info Status info",
                                        "Waiting for other transactions to finish"));
            setProgress(-1);
            break;
        case QApt::WaitingLockStatus:
            setStatusMessage(i18nc("@info Status information, widget title",
                                        "Waiting"));
            setStatusDetail(i18nc("@info Status info",
                                        "Waiting for other software managers to quit"));
            setProgress(-1);
            break;
        case QApt::WaitingMediumStatus:
            setStatusMessage(i18nc("@info Status information, widget title",
                                        "Waiting"));
            setStatusDetail(i18nc("@info Status info",
                                        "Waiting for required medium"));
            setProgress(-1);
            break;
        case QApt::WaitingConfigFilePromptStatus:
            setStatusMessage(i18nc("@info Status information, widget title",
                                        "Waiting"));
            setStatusDetail(i18nc("@info Status info",
                                        "Waiting for configuration file"));
            setProgress(-1);
            break;
        case QApt::RunningStatus:
            setStatusMessage(QString());
            setStatusDetail(QString());
            setProgress(-1);
            break;
        case QApt::LoadingCacheStatus:
            setStatusDetail(QString());
            setStatusMessage(i18nc("@info Status info",
                                        "Loading Software List"));
            setProgress(-1);
            break;
        case QApt::DownloadingStatus:
            setProgress(-1);
            switch (m_trans->role()) {
                case QApt::UpdateCacheRole:
                    setStatusMessage(i18nc("@info Status information, widget title",
                                                "Updating software sources"));
                    break;
                case QApt::DownloadArchivesRole:
                case QApt::CommitChangesRole:
                    setStatusMessage(i18nc("@info Status information, widget title",
                                                "Downloading Packages"));
                    break;
                default:
                    break;
            }
            break;
        case QApt::CommittingStatus:
            setProgress(-1);
            setStatusMessage(i18nc("@info Status information, widget title",
                                        "Applying Changes"));
            setStatusDetail(QString());
            break;
        case QApt::FinishedStatus:
            setProgress(100);
            setStatusMessage(i18nc("@info Status information, widget title",
                                        "Finished"));
            m_lastRealProgress = 0;
            break;
    }
}

void ApplicationUpdates::setProgressing(bool progressing)
{
    if(progressing!=m_progressing) {
        m_progressing = progressing;
        emit progressingChanged(progressing);

        if(m_progressing)
            setProgress(-1);
        else
            m_aptBackend->markPackages(m_aptBackend->markedPackages(), QApt::Package::ToKeep);
    }
}

void ApplicationUpdates::setStatusDetail(const QString& msg)
{
    if(m_statusDetail!=msg) {
        m_statusDetail = msg;
        emit statusDetailChanged(msg);
    }
}

void ApplicationUpdates::setStatusMessage(const QString& msg)
{
    if(m_statusMessage!=msg) {
        m_statusMessage = msg;
        emit statusMessageChanged(msg);
    }
}

QString ApplicationUpdates::statusDetail() const
{
    return m_statusDetail;
}

QString ApplicationUpdates::statusMessage() const
{
    return m_statusMessage;
}

void ApplicationUpdates::cancel()
{
    Q_ASSERT(m_trans->isCancellable());
    m_trans->cancel();
}

quint64 ApplicationUpdates::downloadSpeed() const
{
    return m_trans->downloadSpeed();
}

QList<QAction*> ApplicationUpdates::messageActions() const
{
    QList<QAction*> ret;
    //high priority
    ret += QAptActions::self()->actionCollection()->action("dist-upgrade");

    //normal priority
    ret += QAptActions::self()->actionCollection()->action("update");

    //low priority
    ret += QAptActions::self()->actionCollection()->action("software_properties");
    ret += QAptActions::self()->actionCollection()->action("load_archives");
    ret += QAptActions::self()->actionCollection()->action("save_package_list");
    ret += QAptActions::self()->actionCollection()->action("download_from_list");
    ret += QAptActions::self()->actionCollection()->action("history");
    Q_ASSERT(!ret.contains(nullptr));
    return ret;
}

void ApplicationUpdates::transactionFinished(QApt::ExitStatus status)
{
    m_lastRealProgress = 0;
    m_appBackend->reload();
    setProgressing(false);
}

void ApplicationUpdates::calculateUpdates()
{
    auto changes = m_aptBackend->stateChanges(m_updatesCache, QApt::PackageList());
    for(auto pkgList : changes.values()) {
        for(QApt::Package* it : pkgList) {
            AbstractResource* res = m_appBackend->resourceByPackageName(it->name());
            if(!res) //If we couldn't find it by its name, try with
                res = m_appBackend->resourceByPackageName(QString("%1:%2").arg(it->name()).arg(it->architecture()));
            Q_ASSERT(res);
            m_toUpdate += res;
        }
    }
}
