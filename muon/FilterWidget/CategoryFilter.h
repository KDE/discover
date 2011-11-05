#ifndef CATEGORYFILTER_H
#define CATEGORYFILTER_H

#include "FilterModel.h"

namespace QApt {
    class Backend;
}

class CategoryFilter : public FilterModel
{
public:
    CategoryFilter(QObject *parent, QApt::Backend *backend);

    void populate();

private:
    QApt::Backend *m_backend;
};

#endif // CATEGORYFILTER_H
