/***************************************************************************
 *   Copyright © 2010-2012 Jonathan Thomas <echidnaman@kubuntu.org>        *
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

#ifndef RESOURCEEXTENDER_H
#define RESOURCEEXTENDER_H

#include <QtGui/QWidget>

#include "resources/AbstractResourcesBackend.h"
#include "Transaction/Transaction.h"

class QProgressBar;
class QPushButton;

class ResourceExtender : public QWidget
{
    Q_OBJECT
public:
    ResourceExtender(QWidget *parent, AbstractResource *app);

    void setShowInfoButton(bool show);

private:
    AbstractResource *m_resource;
    QPushButton *m_infoButton;
    QPushButton *m_actionButton;
    QPushButton *m_cancelButton;

private Q_SLOTS:
    void setupTransaction(Transaction *trans);
    void transactionCancelled(Transaction *trans);
    void emitInfoButtonClicked();
    void removeButtonClicked();
    void installButtonClicked();
    void cancelButtonClicked();

Q_SIGNALS:
    void infoButtonClicked(AbstractResource *resource);
};

#endif
