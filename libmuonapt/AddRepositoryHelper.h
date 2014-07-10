#ifndef ADDREPOSITORYHELPER_H
#define ADDREPOSITORYHELPER_H

#include <kauthactionreply.h>

using namespace KAuth;

class AddRepositoryHelper : public QObject
{
    Q_OBJECT
public slots:
    ActionReply modify(QVariantMap args);
};

#endif //ADDREPOSITORYHELPER_H