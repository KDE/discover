/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef APPLICATIONADDONSMODEL_H
#define APPLICATIONADDONSMODEL_H

#include <QAbstractListModel>
#include <resources/PackageState.h>
#include "Transaction/AddonList.h"

#include "discovercommon_export.h"

class Transaction;
class AbstractResource;

class DISCOVERCOMMON_EXPORT ApplicationAddonsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(AbstractResource* application READ application WRITE setApplication NOTIFY applicationChanged)
    Q_PROPERTY(bool hasChanges READ hasChanges NOTIFY stateChanged)
    Q_PROPERTY(bool isEmpty READ isEmpty NOTIFY applicationChanged)
    public:
        enum Roles {
            PackageNameRole = Qt::UserRole,
        };

        explicit ApplicationAddonsModel(QObject* parent = nullptr);
        
        AbstractResource* application() const;
        void setApplication(AbstractResource* app);
        bool hasChanges() const;
        
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        QHash<int, QByteArray> roleNames() const override;
        bool isEmpty() const;

    public Q_SLOTS:
        void discardChanges();
        void applyChanges();
        void changeState(const QString& packageName, bool installed);

    Q_SIGNALS:
        void stateChanged();
        void applicationChanged();

    private:
        void transactionOver(Transaction* t);
        void resetState();

        AbstractResource* m_app;
        QList<PackageState> m_initial;
        AddonList m_state;
};

#endif // APPLICATIONADDONSMODEL_H
