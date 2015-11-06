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

#ifndef MUONSTRINGS_H
#define MUONSTRINGS_H

#include <QtCore/QHash>

#include <QApt/Package>

namespace QApt {
    class Transaction;
}

class MuonStrings : public QObject
{
    Q_OBJECT
public:
    explicit MuonStrings(QObject *parent);

    static MuonStrings* global();

    QString groupName(const QString &name) const;
    QString groupKey(const QString &text) const;

    /** @returns the state name for a given @p state, for displaying it to the user */
    QString packageStateName(QApt::Package::State state) const;

    /** @returns the state name for the given @p state changes, for displaying it to the user
     * This means, the flags that are related to a state change, like ToInstall, ToUpgrade, etc
     */
    QString packageChangeStateName(QApt::Package::State state) const;
    QString archString(const QString &arch) const;
    QString errorTitle(QApt::ErrorCode error) const;
    QString errorText(QApt::ErrorCode error, QApt::Transaction *trans) const;

private:
    const QHash<QString, QString> m_groupHash;
    const QHash<int, QString> m_stateHash;
    const QHash<QString, QString> m_archHash;

    QHash<QString, QString> groupHash();
    QHash<int, QString> stateHash();
    QHash<QString, QString> archHash();
};

#endif
