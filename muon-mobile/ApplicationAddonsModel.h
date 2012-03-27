/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#ifndef APPLICATIONADDONSMODEL_H
#define APPLICATIONADDONSMODEL_H

#include <QAbstractListModel>

namespace QApt { class Package; }
class Application;

class ApplicationAddonsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(Application* application READ application WRITE setApplication)
    Q_PROPERTY(bool hasChanges READ hasChanges NOTIFY stateChanged)
    public:
        explicit ApplicationAddonsModel(QObject* parent = 0);
        
        Application* application() const;
        void setApplication(Application* app);
        bool hasChanges() const;
        
        virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
        virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;

    public slots:
        void discardChanges();
        void applyChanges();
        void changeState(const QString& packageName, bool installed);

    signals:
        void stateChanged();

    private:
        Application* m_app;
        QList<QApt::Package*> m_addons;
        QMap<QString, bool> m_state;
};

#endif // APPLICATIONADDONSMODEL_H
