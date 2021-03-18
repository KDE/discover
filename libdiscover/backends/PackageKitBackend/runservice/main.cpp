/*
 *   SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <KIO/ApplicationLauncherJob>
#include <KService>
#include <QFile>
#include <QGuiApplication>
#include <QProcess>
#include <QTextStream>
#include <QUrl>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    if (app.arguments().size() != 2)
        return 1;

    KService::Ptr service(new KService(app.arguments().constLast()));
    if (!service->isValid())
        return 2;

    KIO::ApplicationLauncherJob *job = new KIO::ApplicationLauncherJob(service);
    job->start();
    QObject::connect(job, &KIO::ApplicationLauncherJob::finished, &app, [job] {
        QCoreApplication::instance()->exit(job->error());
    });
    return app.exec();
}
