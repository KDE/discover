#include "AddRepositoryHelper.h"
#include <QProcess>
#include <QDebug>
#include <unistd.h>
#include <stdlib.h>
#include <kauthhelpersupport.h>

ActionReply AddRepositoryHelper::modify(QVariantMap args)
{
    ActionReply reply = ActionReply::SuccessReply();
    if(args[QStringLiteral("repository")].isNull() || args[QStringLiteral("action")].isNull()) {
        reply.setErrorDescription(QStringLiteral("Invalid action arguments."));
        reply = ActionReply::HelperErrorReply();
        return reply;
    }
    QProcess *p = new QProcess(this);
    p->setProcessChannelMode(QProcess::MergedChannels);
    QString modRepo(QStringLiteral("apt-add-repository"));
    QStringList arguments;
    if(args[QStringLiteral("action")].toString() == QLatin1String("add")) {
        arguments.append(QStringLiteral("-y"));
        arguments.append(args[QStringLiteral("repository")].toString());
    } else {
        if(args[QStringLiteral("action")] == QLatin1String("remove"))
        {
            arguments.append(QStringLiteral("--remove"));
            arguments.append(QStringLiteral("-y"));
            arguments.append(args[QStringLiteral("repository")].toString());
        }
    }
    p->start(modRepo,arguments);
    p->waitForFinished();
    if(p->exitCode() != 0) {
        reply.setErrorDescription(QStringLiteral("Could not modify source."));
        reply= ActionReply::HelperErrorReply();
    }
    p->deleteLater();
    return reply;
}

KAUTH_HELPER_MAIN("org.kde.muon.repo", AddRepositoryHelper)
