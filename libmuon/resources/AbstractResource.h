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
        
        ///used as internal identification of a resource
        virtual QString packageName() const = 0;
        
        ///resource name to be displayed
        virtual QString name() = 0;
};

#endif // ABSTRACTRESOURCE_H
