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

#include "MainTab.h"

// Qt includes
#include <QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

// KDE includes
#include <KDebug>
#include <KDialog>
#include <KHBox>
#include <KLocalizedString>
#include <KMenu>
#include <KMessageBox>
#include <KTextBrowser>

// QApt includes
#include <QApt/Backend>
#include <QApt/Package>

MainTab::MainTab(QWidget *parent)
    : DetailsTab(parent)
{
    m_name = i18nc("@title:tab", "Details");

    KHBox *headerBox = new KHBox(this);
    m_packageShortDescLabel = new QLabel(headerBox);
    m_packageShortDescLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    QFont font;
    font.setBold(true);
    m_packageShortDescLabel->setFont(font);

    QWidget *buttonBox = new QWidget(headerBox);

    QHBoxLayout *buttonBoxLayout = new QHBoxLayout(buttonBox);
    buttonBoxLayout->setMargin(0);
    buttonBoxLayout->setSpacing(KDialog::spacingHint());

    m_buttonLabel = new QLabel(buttonBox);
    m_buttonLabel->setText(i18nc("@label", "Mark for:"));
    buttonBoxLayout->addWidget(m_buttonLabel);

    m_installButton = new QPushButton(buttonBox);
    m_installButton->setIcon(QIcon::fromTheme("download"));
    m_installButton->setText(i18nc("@action:button", "Installation"));
    connect(m_installButton, SIGNAL(clicked()), this, SLOT(emitSetInstall()));
    buttonBoxLayout->addWidget(m_installButton);

    m_removeButton = new QToolButton(buttonBox);
    m_removeButton->setPopupMode(QToolButton::MenuButtonPopup);
    m_removeButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_removeButton->setIcon(QIcon::fromTheme("edit-delete"));
    m_removeButton->setText(i18nc("@action:button", "Removal"));
    connect(m_removeButton, SIGNAL(clicked()), this, SLOT(emitSetRemove()));
    buttonBoxLayout->addWidget(m_removeButton);

    m_upgradeButton = new QPushButton(buttonBox);
    m_upgradeButton->setIcon(QIcon::fromTheme("system-software-update"));
    m_upgradeButton->setText(i18nc("@action:button", "Upgrade"));
    connect(m_upgradeButton, SIGNAL(clicked()), this, SLOT(emitSetInstall()));
    buttonBoxLayout->addWidget(m_upgradeButton);

    m_reinstallButton = new QPushButton(buttonBox);
    m_reinstallButton->setIcon(QIcon::fromTheme("view-refresh"));
    m_reinstallButton->setText(i18nc("@action:button", "Reinstallation"));
    connect(m_reinstallButton, SIGNAL(clicked()), this, SLOT(emitSetReInstall()));
    buttonBoxLayout->addWidget(m_reinstallButton);

    m_purgeMenu = new KMenu(m_removeButton);
    m_purgeAction = new QAction(this);
    m_purgeAction->setIcon(QIcon::fromTheme("edit-delete-shred"));
    m_purgeAction->setText(i18nc("@action:button", "Purge"));
    connect(m_purgeAction, SIGNAL(triggered()), this, SLOT(emitSetPurge()));
    m_purgeMenu->addAction(m_purgeAction);
    m_removeButton->setMenu(m_purgeMenu);

    m_purgeButton = new QPushButton(buttonBox);
    m_purgeButton->setIcon(m_purgeAction->icon());
    m_purgeButton->setText(m_purgeAction->text());
    connect(m_purgeButton, SIGNAL(clicked()), this, SLOT(emitSetPurge()));
    buttonBoxLayout->addWidget(m_purgeButton);

    m_cancelButton = new QPushButton(buttonBox);
    m_cancelButton->setIcon(QIcon::fromTheme("dialog-cancel"));
    m_cancelButton->setText(i18nc("@action:button", "Unmark"));
    connect(m_cancelButton, SIGNAL(clicked()), this, SLOT(emitSetKeep()));
    buttonBoxLayout->addWidget(m_cancelButton);

    m_descriptionBrowser = new KTextBrowser(this);

    m_layout->addWidget(headerBox);
    m_layout->addWidget(m_descriptionBrowser);
}

void MainTab::hideButtons()
{
    m_installButton->hide();
    m_removeButton->hide();
    m_upgradeButton->hide();
    m_reinstallButton->hide();
    m_cancelButton->hide();
    m_buttonLabel->hide();

}

void MainTab::refresh()
{
    if (!m_package) {
        return; // Nothing to refresh yet, so return, else we crash
    }
    m_buttonLabel->show();
    int state = m_package->state();
    bool upgradeable = (state & QApt::Package::Upgradeable);

    if (state & QApt::Package::Installed) {
        m_installButton->hide();
        m_removeButton->show();
        if (upgradeable) {
            m_upgradeButton->show();
        } else {
            m_upgradeButton->hide();
        }
        if (state & (QApt::Package::NotDownloadable) || upgradeable) {
            m_reinstallButton->hide();
        } else {
            m_reinstallButton->show();
        }
        m_cancelButton->hide();
        m_purgeButton->hide();
    } else if (state & QApt::Package::ResidualConfig) {
        m_purgeButton->show();
        m_installButton->show();
        m_removeButton->hide();
        m_upgradeButton->hide();
        m_reinstallButton->hide();
        m_cancelButton->hide();
    } else {
        m_installButton->show();
        m_removeButton->hide();
        m_upgradeButton->hide();
        m_reinstallButton->hide();
        m_purgeButton->hide();
        m_cancelButton->hide();
    }

    // If status changed
    if (state & (QApt::Package::ToInstall | QApt::Package::ToReInstall |
                 QApt::Package::ToUpgrade | QApt::Package::ToDowngrade |
                 QApt::Package::ToRemove  | QApt::Package::ToPurge)) {
        m_installButton->hide();
        m_removeButton->hide();
        m_upgradeButton->hide();
        m_reinstallButton->hide();
        m_cancelButton->show();
    }

    m_packageShortDescLabel->setText(m_package->shortDescription());

    m_descriptionBrowser->setText(m_package->longDescription());

    // Append a newline to give a bit of separation for the support string
    m_descriptionBrowser->append(QString());
    if (m_package->isSupported()) {
        QDateTime endDate = m_package->supportedUntil();
        m_descriptionBrowser->append(i18nc("@info Tells how long Canonical, Ltd. will support a package",
                                        "Canonical provides critical updates for %1 until %2.",
                                        m_package->name(), endDate.date().toString()));
    } else {
        m_descriptionBrowser->append(i18nc("@info Tells how long Canonical, Ltd. will support a package",
                                        "Canonical does not provide updates for %1. Some updates "
                                        "may be provided by the Ubuntu community.", m_package->name()));
    }
}

void MainTab::emitSetInstall()
{
    emit setInstall(m_package);
}

void MainTab::emitSetRemove()
{
    emit setRemove(m_package);
}

void MainTab::emitSetUpgrade()
{
    emit setUpgrade(m_package);
}

void MainTab::emitSetReInstall()
{
    emit setReInstall(m_package);
}

void MainTab::emitSetKeep()
{
    emit setKeep(m_package);
}

void MainTab::emitSetPurge()
{
    emit setPurge(m_package);
}

#include "MainTab.moc"
