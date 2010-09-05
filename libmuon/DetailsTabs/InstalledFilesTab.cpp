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

#include "InstalledFilesTab.h"

// KDE includes
#include <KTextBrowser>

// LibQApt includes
#include <LibQApt/Package>

InstalledFilesTab::InstalledFilesTab(QWidget *parent)
    : KVBox(parent)
    , m_package(0)
{
    m_filesBrowser = new KTextBrowser(this);
}

InstalledFilesTab::~InstalledFilesTab()
{
}

void InstalledFilesTab::setPackage(QApt::Package *package)
{
    m_package = package;
    populateFilesList();
}

void InstalledFilesTab::populateFilesList()
{
    m_filesBrowser->clear();
    QStringList filesList = m_package->installedFilesList();
    QString filesString;

    foreach(const QString & file, filesList) {
        filesString.append(file + '\n');
    }

    m_filesBrowser->setPlainText(filesString);
}

#include "InstalledFilesTab.moc"
