/***************************************************************************
 *   Copyright © 2020 Alexey Min <alexey.min@gmail.com>                    *
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

#ifndef ALPINEAPKSOURCESBACKEND_H
#define ALPINEAPKSOURCESBACKEND_H

#include <resources/AbstractSourcesBackend.h>
#include <QStandardItemModel>

class AlpineApkSourcesBackend : public AbstractSourcesBackend
{
public:
    explicit AlpineApkSourcesBackend(AbstractResourcesBackend *parent);

    QAbstractItemModel *sources() override;
    bool addSource(const QString &id) override;
    bool removeSource(const QString &id) override;
    QString idDescription() override;
    QVariantList actions() const override;
    bool supportsAdding() const override;
    bool canMoveSources() const override;
    bool moveSource(const QString &sourceId, int delta) override;

private:
    QStandardItem *sourceForId(const QString &id) const;
    bool addSourceFull(const QString &id, const QString &comment, bool enabled);
    void loadSources();

    QStandardItemModel *m_sourcesModel = nullptr;
    QAction *m_refreshAction = nullptr;
};

#endif // ALPINEAPKSOURCESBACKEND_H
