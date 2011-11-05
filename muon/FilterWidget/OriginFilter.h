#ifndef ORIGINFILTER_H
#define ORIGINFILTER_H

#include "FilterModel.h"

namespace QApt {
    class Backend;
}

class OriginFilter : public FilterModel
{
public:
    OriginFilter(QObject *parent, QApt::Backend *backend);

    void populate();
    void reload();

private:
    QApt::Backend *m_backend;
};

#endif // ORIGINFILTER_H
