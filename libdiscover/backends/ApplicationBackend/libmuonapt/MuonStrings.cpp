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

#include "MuonStrings.h"

#include <KLocalizedString>
#include <QDebug>

#include <QApt/Transaction>

Q_GLOBAL_STATIC_WITH_ARGS(MuonStrings, globalMuonStrings, (0))

using namespace QApt;

MuonStrings *MuonStrings::global()
{
    return globalMuonStrings;
}

MuonStrings::MuonStrings(QObject *parent)
    : QObject(parent)
    , m_stateHash(stateHash())
{
}

QHash<int, QString> MuonStrings::stateHash()
{
    QHash<int, QString> hash;
    hash[Package::ToKeep] = i18nc("@info:status Package state", "No Change");
    hash[Package::ToInstall] = i18nc("@info:status Requested action", "Install");
    hash[Package::NewInstall] = i18nc("@info:status Requested action", "Install");
    hash[Package::ToReInstall] = i18nc("@info:status Requested action", "Reinstall");
    hash[Package::ToUpgrade] = i18nc("@info:status Requested action", "Upgrade");
    hash[Package::ToDowngrade] = i18nc("@info:status Requested action", "Downgrade");
    hash[Package::ToRemove] = i18nc("@info:status Requested action", "Remove");
    hash[Package::Held] = i18nc("@info:status Package state" , "Held");
    hash[Package::Installed] = i18nc("@info:status Package state", "Installed");
    hash[Package::Upgradeable] = i18nc("@info:status Package state", "Upgradeable");
    hash[Package::NowBroken] = i18nc("@info:status Package state", "Broken");
    hash[Package::InstallBroken] = i18nc("@info:status Package state", "Install Broken");
    hash[Package::Orphaned] = i18nc("@info:status Package state", "Orphaned");
    hash[Package::Pinned] = i18nc("@info:status Package state", "Locked");
    hash[Package::New] = i18nc("@info:status Package state", "New in repository");
    hash[Package::ResidualConfig] = i18nc("@info:status Package state", "Residual Configuration");
    hash[Package::NotDownloadable] = i18nc("@info:status Package state", "Not Downloadable");
    hash[Package::ToPurge] = i18nc("@info:status Requested action", "Purge");
    hash[Package::IsImportant] = i18nc("@info:status Package state", "Important for base install");
    hash[Package::OverrideVersion] = i18nc("@info:status Package state", "Version overridden");
    hash[Package::IsAuto] = i18nc("@info:status Package state", "Required by other packages");
    hash[Package::IsGarbage] = i18nc("@info:status Package state", "Installed (auto-removable)");
    hash[Package::NowPolicyBroken] = i18nc("@info:status Package state", "Policy Broken");
    hash[Package::InstallPolicyBroken] = i18nc("@info:status Package state", "Policy Broken");
    hash[Package::NotInstalled] = i18nc("@info:status Package state" , "Not Installed");
    hash[Package::IsPinned] = i18nc("@info:status Package locked at a certain version",
                                          "Locked");
    hash[Package::IsManuallyHeld] = i18nc("@info:status Package state", "Manually held back");

    return hash;
}

QString MuonStrings::packageStateName(Package::State state) const
{
    return m_stateHash.value(state);
}

QString MuonStrings::packageChangeStateName(Package::State state) const
{
    int ns = state & (Package::ToKeep | Package::ToInstall | Package::ToReInstall | Package::NewInstall
                                    | Package::ToUpgrade | Package::ToRemove
                                    | Package::ToPurge | Package::ToDowngrade);
    return m_stateHash.value(ns);
}

QString MuonStrings::errorTitle(ErrorCode error) const
{
    switch (error) {
    case InitError:
        return i18nc("@title:window", "Initialization Error");
    case LockError:
        return i18nc("@title:window", "Unable to Obtain Package System Lock");
    case DiskSpaceError:
        return i18nc("@title:window", "Low Disk Space");
    case FetchError:
    case CommitError:
        return i18nc("@title:window", "Failed to Apply Changes");
    case AuthError:
        return i18nc("@title:window", "Authentication error");
    case WorkerDisappeared:
        return i18nc("@title:window", "Unexpected Error");
    case UntrustedError:
        return i18nc("@title:window", "Untrusted Packages");
    case UnknownError:
    default:
        return i18nc("@title:window", "Unknown Error");
    }
}

QString MuonStrings::errorText(ErrorCode error, Transaction *trans) const
{
    QString text;

    switch (error) {
    case InitError:
        text = i18nc("@label", "The package system could not be initialized, your "
                               "configuration may be broken.");
        break;
    case LockError:
        text = i18nc("@label",
                     "Another application seems to be using the package "
                     "system at this time. You must close all other package "
                     "managers before you will be able to install or remove "
                     "any packages.");
        break;
    case DiskSpaceError:
        text = i18nc("@label",
                     "You do not have enough disk space in the directory "
                     "at %1 to continue with this operation.", trans->errorDetails());
        break;
    case FetchError:
        text = i18nc("@label", "Could not download packages");
        break;
    case CommitError:
        text = i18nc("@label", "An error occurred while applying changes:");
        break;
    case AuthError:
        text = i18nc("@label",
                     "This operation cannot continue since proper "
                     "authorization was not provided");
        break;
    case WorkerDisappeared:
        text = i18nc("@label", "It appears that the QApt worker has either crashed "
                     "or disappeared. Please report a bug to the QApt maintainers");
        break;
    case UntrustedError:
        text = i18ncp("@label",
                      "The following package has not been verified by its author. "
                      "Downloading untrusted packages has been disallowed "
                      "by your current configuration.",
                      "The following packages have not been verified by "
                      "their authors. "
                      "Downloading untrusted packages has "
                      "been disallowed by your current configuration.",
                      trans->untrustedPackages().size());
        break;
    default:
        break;
    }

    return text;
}
