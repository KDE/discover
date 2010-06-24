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

#include "CommitWidget.h"

// Qt includes
#include <QtGui/QLabel>
#include <QtGui/QProgressBar>
#include <QtGui/QVBoxLayout>

#include <DebconfGui.h>

CommitWidget::CommitWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    setLayout(layout);

    layout->addStretch();

    m_debconfGui = new DebconfKde::DebconfGui("/tmp/qapt-sock", this);
    layout->addWidget(m_debconfGui);
    m_debconfGui->connect(m_debconfGui, SIGNAL(activated()), m_debconfGui, SLOT(show()));
    m_debconfGui->connect(m_debconfGui, SIGNAL(deactivated()), m_debconfGui, SLOT(hide()));
    m_debconfGui->hide();

    m_commitLabel = new QLabel(this);
    layout->addWidget(m_commitLabel);

    m_progressBar = new QProgressBar(this);
    layout->addWidget(m_progressBar);

    layout->addStretch();
}

CommitWidget::~CommitWidget()
{
}

void CommitWidget::setLabelText(const QString &text)
{
    m_commitLabel->setText(text);
}

void CommitWidget::setProgress(int percentage)
{
    m_progressBar->setValue(percentage);
}

void CommitWidget::updateCommitProgress(const QString& message, int percentage)
{
    setLabelText(message);
    setProgress(percentage);
}

void CommitWidget::clear()
{
    m_commitLabel->setText(QString());
    m_progressBar->setValue(0);
}

#include "CommitWidget.moc"
