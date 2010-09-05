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

#ifndef MAINTAB_H
#define MAINTAB_H

// Qt includes
#include <QtCore/QVariantMap>
#include <QtGui/QWidget>

// LibQApt includes
#include <LibQApt/Globals>

class QPushButton;
class QLabel;
class QToolButton;

class KAction;
class KJob;
class KMenu;
class KPixmapSequenceWidget;
class KTemporaryFile;
class KTextBrowser;

namespace QApt
{
    class Backend;
    class Package;
}

class MainTab : public QWidget
{
    Q_OBJECT
public:
    explicit MainTab(QWidget *parent);
    ~MainTab();

private:
    QApt::Backend *m_backend;
    QApt::Package *m_package;
    QApt::CacheState m_oldCacheState;

    QLabel *m_packageShortDescLabel;
    KPixmapSequenceWidget *m_throbberWidget;
    QPushButton *m_screenshotButton;

    QPushButton *m_installButton;
    QToolButton *m_removeButton;
    QPushButton *m_upgradeButton;
    QPushButton *m_reinstallButton;
    KAction *m_purgeAction;
    KMenu *m_purgeMenu;
    QPushButton *m_purgeButton;
    QPushButton *m_cancelButton;

    KTextBrowser *m_descriptionBrowser;
    KTemporaryFile *m_screenshotFile;

    QLabel *m_supportedLabel;

    QString digestReason(QApt::BrokenReason failType, QHash<QString, QVariantMap> failReason);

public Q_SLOTS:
    void setBackend(QApt::Backend *backend);
    void setPackage(QApt::Package *package);
    void refresh();
    void clear();

private Q_SLOTS:
    void fetchScreenshot();
    void screenshotFetched(KJob *job);
    bool confirmEssentialRemoval();
    void setInstall();
    void setRemove();
    void setUpgrade();
    void setReInstall();
    void setPurge();
    void setKeep();
    void showBrokenReason();
};

#endif
