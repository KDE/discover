/*
 * SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 *
 */

#ifndef KDEVPLATFORM_UNITYLAUNCHER_H
#define KDEVPLATFORM_UNITYLAUNCHER_H

#include <QObject>

class UnityLauncher : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString launcherId READ launcherId WRITE setLauncherId)
    Q_PROPERTY(bool progressVisible READ progressVisible WRITE setProgressVisible)
    Q_PROPERTY(int progress READ progress WRITE setProgress)

public:
    explicit UnityLauncher(QObject *parent = nullptr);
    ~UnityLauncher() override;

    QString launcherId() const;
    void setLauncherId(const QString &launcherId);

    bool progressVisible() const;
    void setProgressVisible(bool progressVisible);

    int progress() const;
    void setProgress(int progress);

private:
    void update(const QVariantMap &properties);

    QString m_launcherId;
    bool m_progressVisible = false;
    int m_progress = 0;

};

#endif // KDEVPLATFORM_UNITYLAUNCHER_H
