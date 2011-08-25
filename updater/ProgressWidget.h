#ifndef PROGRESSWIDGET_H
#define PROGRESSWIDGET_H

#include <QtGui/QWidget>

class QLabel;
class QParallelAnimationGroup;
class QProgressBar;

class ProgressWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ProgressWidget(QWidget *parent = 0);

private:
    QLabel *m_statusLabel;
    QProgressBar *m_progressBar;
    QLabel *m_detailsLabel;

    QParallelAnimationGroup *m_expandWidget;

public Q_SLOTS:
    void setCommitProgress(int percentage);
    void updateDownloadProgress(int percentage, int speed, int ETA);
    void updateCommitProgress(const QString &message, int percentage);
    void setHeaderText(const QString &text);

    void show();
    void animatedHide();

Q_SIGNALS:
    void cancelDownload();
};

#endif // PROGRESSWIDGET_H
