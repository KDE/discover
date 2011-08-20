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

#include "AddonsWidget.h"

#include <QtCore/QStringBuilder>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QListView>
#include <QtGui/QPushButton>
#include <QtGui/QStandardItemModel>
#include <QtGui/QToolButton>

#include <KIcon>
#include <KLocale>

#include "Application.h"
#include "ApplicationBackend.h"

AddonsWidget::AddonsWidget(QWidget *parent, ApplicationBackend *appBackend)
        : KVBox(parent)
        , m_appBackend(appBackend)
{
    QWidget *headerWidget = new QWidget(this);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setMargin(0);
    headerWidget->setLayout(headerLayout);

    m_expandButton = new QToolButton(headerWidget);
    m_expandButton->setAutoRaise(true);
    m_expandButton->setArrowType(Qt::DownArrow);
    connect(m_expandButton, SIGNAL(clicked()), this, SLOT(expandButtonClicked()));

    QLabel *titleLabel = new QLabel(headerWidget);
    titleLabel->setText(QLatin1Literal("<h3>") %
                        i18nc("@title", "Addons") % QLatin1Literal("</h3>"));

    QWidget *headerSpacer = new QWidget(headerWidget);
    headerSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    headerLayout->addWidget(m_expandButton);
    headerLayout->addWidget(titleLabel);
    headerLayout->addWidget(headerSpacer);

    m_addonsWidget = new QWidget(this);
    QVBoxLayout *addonsLayout = new QVBoxLayout(m_addonsWidget);
    addonsLayout->setMargin(0);
    m_addonsWidget->setLayout(addonsLayout);

    m_addonsView = new QListView(m_addonsWidget);
    m_addonsView->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    m_addonsModel = new QStandardItemModel(this);
    m_addonsView->setModel(m_addonsModel);

    QWidget *addonsButtonBox = new QWidget(m_addonsWidget);
    QHBoxLayout *addonButtonsLayout = new QHBoxLayout(addonsButtonBox);

    QWidget *addonsButtonSpacer = new QWidget(m_addonsWidget);
    addonsButtonSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    m_addonsRevertButton = new QPushButton(addonsButtonBox);
    m_addonsRevertButton->setIcon(KIcon("edit-undo"));
    m_addonsRevertButton->setText(i18nc("@action:button", "Revert"));
    connect(m_addonsRevertButton, SIGNAL(clicked()),
            this, SLOT(populateModel()));

    m_addonsApplyButton = new QPushButton(addonsButtonBox);
    m_addonsApplyButton->setIcon(KIcon("dialog-ok-apply"));
    m_addonsApplyButton->setText(i18nc("@action:button", "Apply"));
    m_addonsApplyButton->setToolTip(i18nc("@info:tooltip", "Apply changes to addons"));
    connect(m_addonsApplyButton, SIGNAL(clicked()),
            this, SLOT(emitApplyButtonClicked()));

    addonButtonsLayout->addWidget(addonsButtonSpacer);
    addonButtonsLayout->addWidget(m_addonsRevertButton);
    addonButtonsLayout->addWidget(m_addonsApplyButton);

    addonsLayout->addWidget(m_addonsView);
    addonsLayout->addWidget(addonsButtonBox);
}

AddonsWidget::~AddonsWidget()
{
}

void AddonsWidget::setAddons(QApt::PackageList addons)
{
    m_availableAddons = addons;

    connect(m_addonsModel, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
                this, SLOT(addonStateChanged(const QModelIndex &, const QModelIndex &)));

    populateModel();
}

void AddonsWidget::repaintViewport()
{
    m_addonsView->viewport()->update();
    m_addonsView->viewport()->repaint();
}

void AddonsWidget::populateModel()
{
    m_addonsModel->clear();

    foreach (QApt::Package *addon, m_availableAddons) {
        // Check if we have an application for the addon
        Application *addonApp = 0;

        foreach (Application *app, m_appBackend->applicationList()) {
            if (app->package()->latin1Name() == addon->latin1Name()) {
                addonApp = app;
                break;
            }
        }

        QStandardItem *addonItem = new QStandardItem;
        addonItem->setData(addon->latin1Name());
        QString packageName = QLatin1Literal(" (") % addon->latin1Name() % ')';
        if (addonApp) {
            addonItem->setText(addonApp->name() % packageName);
            addonItem->setIcon(KIcon(addonApp->icon()));
        } else {
            addonItem->setText(addon->shortDescription() % packageName);
            addonItem->setIcon(KIcon("applications-other"));
        }

        addonItem->setEditable(false);
        addonItem->setCheckable(true);

        if (addon->isInstalled()) {
            addonItem->setCheckState(Qt::Checked);
        } else {
            addonItem->setCheckState(Qt::Unchecked);
        }

        m_addonsModel->appendRow(addonItem);
    }

    m_addonsRevertButton->setEnabled(false);
    m_addonsApplyButton->setEnabled(false);
}

void AddonsWidget::expandButtonClicked()
{
    if (m_addonsWidget->isHidden()) {
        m_expandButton->setArrowType(Qt::DownArrow);
        m_addonsWidget->show();
    } else {
        m_addonsWidget->hide();
        m_expandButton->setArrowType(Qt::RightArrow);
    }
}

void AddonsWidget::addonStateChanged(const QModelIndex &left, const QModelIndex &right)
{
    Q_UNUSED(right);
    QStandardItem *item = m_addonsModel->itemFromIndex(left);
    QApt::Package *addon = 0;

    QString addonName = item->data().toString();
    foreach(QApt::Package *pkg, m_availableAddons) {
        if (pkg->latin1Name() == addonName) {
            addon = pkg;
        }
    }

    if (!addon) {
        return;
    }

    if (addon->isInstalled()) {
        switch (item->checkState()) {
        case Qt::Checked:
            if (m_changedAddons.contains(addon)) {
                m_changedAddons.remove(addon);
            }
            break;
        case Qt::Unchecked:
            m_changedAddons[addon] = QApt::Package::ToRemove;
            break;
        default:
            break;
        }
    } else {
        switch (item->checkState()) {
        case Qt::Checked:
            m_changedAddons[addon] = QApt::Package::ToInstall;
            break;
        case Qt::Unchecked:
            if (m_changedAddons.contains(addon)) {
                m_changedAddons.remove(addon);
            }
            break;
        default:
            break;
        }
    }

    m_addonsRevertButton->setEnabled(!m_changedAddons.isEmpty());
    m_addonsApplyButton->setEnabled(!m_changedAddons.isEmpty());
}

void AddonsWidget::emitApplyButtonClicked()
{
    emit applyButtonClicked(m_changedAddons);
}


#include "AddonsWidget.moc"
