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
    void checkApps(QList<Application *> apps, bool checked);
    void checkApp(Application *app, bool checked);
};

#endif // UPDATERWIDGET_H
