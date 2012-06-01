#ifndef ABSTRACTRESOURCE_H
#define ABSTRACTRESOURCE_H

#include <QtCore/QObject>

class AbstractResource : public QObject
{
    Q_OBJECT
    Q_PROPERTY(AbstractResource* application READ application CONSTANT)
    public:
        explicit AbstractResource(QObject* parent = 0);
        
        AbstractResource* application() { return this; }
};

#endif // ABSTRACTRESOURCE_H
