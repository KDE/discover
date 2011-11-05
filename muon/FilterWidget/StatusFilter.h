#ifndef STATUSFILTER_H
#define STATUSFILTER_H

#include "FilterModel.h"

class StatusFilter : public FilterModel
{
public:
    explicit StatusFilter(QObject *parent = 0);

    void populate();
};

#endif // STATUSFILTER_H
