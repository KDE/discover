#ifndef UPDATERWIDGET_H
#define UPDATERWIDGET_H

#include <QtGui/QWidget>

class QStandardItemModel;
class QTreeView;

namespace QApt {
    class Backend;
}

class Application;
class UpdateModel;

class UpdaterWidget : public QWidget
{
    Q_OBJECT
public:
    explicit UpdaterWidget(QWidget *parent = 0);

    enum UpdateModelRole {
        UpdateTypeRole = Qt::UserRole + 1,
        ListIndexRole = Qt::UserRole + 2
    };

private:
    QApt::Backend *m_backend;
    UpdateModel *m_updateModel;
    QList<Application *> m_upgradeableApps;

    QTreeView *m_updateView;

public Q_SLOTS:
    void setBackend(QApt::Backend *backend);
    void reload();

private Q_SLOTS:
    void populateUpdateModel();
};

#endif // UPDATERWIDGET_H
