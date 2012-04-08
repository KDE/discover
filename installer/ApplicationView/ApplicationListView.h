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

#ifndef APPLICATIONLISTVIEW_H
#define APPLICATIONLISTVIEW_H

#include <QModelIndex>

#include <LibQApt/Package>

#include "AbstractViewContainer.h"

class ApplicationBackend;
class ApplicationViewWidget;

namespace QApt {
    class Backend;
}

class ApplicationListView : public AbstractViewContainer
{
    Q_OBJECT
public:
    ApplicationListView(QWidget *parent, ApplicationBackend *m_appBackend,
                        const QModelIndex &index);
    ~ApplicationListView();

    void setStateFilter(QApt::Package::State state);
    void setOriginFilter(const QString &origin);
    void setShouldShowTechnical(bool show);
    void setCanShowTechnical(bool canShow);

private:
    QApt::Backend *m_backend;
    ApplicationBackend *m_appBackend;

    ApplicationViewWidget *m_appViewWidget;

public Q_SLOTS:
    void setBackend(QApt::Backend *backend);
};

#endif
