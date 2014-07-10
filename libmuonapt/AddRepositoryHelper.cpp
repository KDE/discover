#include "AddRepositoryHelper.h"
#include <QProcess>
#include <QDebug>
#include <unistd.h>
#include <stdlib.h>

ActionReply AddRepositoryHelper::modify(QVariantMap args)
{
    ActionReply reply = ActionReply::SuccessReply();
    if(args["repository"].isNull() || args["action"].isNull()) {
        reply.setErrorDescription("Invalid action arguments.");
        reply = ActionReply::HelperErrorReply();
        return reply;
    }
    QProcess *p = new QProcess(this);
    p->setProcessChannelMode(QProcess::MergedChannels);
    QString modRepo("apt-add-repository");
    QStringList arguments;
    if(args["action"].toString()==QString("add")) {
        arguments.append(QString("-y"));
        arguments.append(args["repository"].toString());
    } else {
        if(args["action"]=="remove")
        {
            arguments.append(QString("--remove"));
            arguments.append(QString("-y"));
            arguments.append(args["repository"].toString());
        }
    }
    p->start(modRepo,arguments);
    p->waitForFinished();
    if(p->exitCode()) {
        reply.setErrorDescription("Could not modify source.");
        reply= ActionReply::HelperErrorReply();
    }
    p->deleteLater();
    return reply;
}

KAUTH_HELPER_MAIN("org.kde.muon.repo", AddRepositoryHelper)
