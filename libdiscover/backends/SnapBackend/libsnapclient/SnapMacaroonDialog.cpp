/*
 *   SPDX-FileCopyrightText: 2017 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "ui_SnapMacaroonDialog.h"
#include <KAuth/ExecuteJob>
#include <QApplication>
#include <QBuffer>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QTextStream>

class MacaroonDialog : public QDialog
{
public:
    MacaroonDialog()
        : QDialog()
    {
        m_ui.setupUi(this);
        connect(this, &QDialog::accepted, this, &MacaroonDialog::startLogin);
        connect(this, &QDialog::rejected, this, []() {
            qApp->exit(1);
        });

        setOtpMode(false);
    }

    void startLogin()
    {
        login(m_ui.username->text(), m_ui.password->text(), m_ui.otp->text());
    }

    void login(const QString &username, const QString &password, const QString &otp = {})
    {
        KAuth::Action snapAction(QStringLiteral("org.kde.discover.libsnapclient.login"));
        snapAction.setHelperId(QStringLiteral("org.kde.discover.libsnapclient"));
        snapAction.setArguments({
            {QStringLiteral("user"), username},
            {QStringLiteral("password"), password},
            {QStringLiteral("otp"), otp},
        });
        Q_ASSERT(snapAction.isValid());

        KAuth::ExecuteJob *reply = snapAction.execute();
        connect(reply, &KAuth::ExecuteJob::result, this, &MacaroonDialog::replied);
        reply->start();
    }

    void setOtpMode(bool enabled)
    {
        m_ui.password->setEnabled(!enabled);
        m_ui.otp->setVisible(enabled);
        m_ui.otpLabel->setVisible(enabled);
    }

    void replied(KJob *job)
    {
        KAuth::ExecuteJob *reply = static_cast<KAuth::ExecuteJob *>(job);
        const QVariantMap replyData = reply->data();
        if (reply->error() == 0) {
            QTextStream(stdout) << replyData[QLatin1String("reply")].toString();
            QCoreApplication::instance()->exit(0);
        } else {
            const QString message = replyData.value(QLatin1String("errorString"), reply->errorString()).toString();
            setOtpMode(replyData[QLatin1String("otpMode")].toBool());

            m_ui.errorMessage->setText(message);
            show();
        }
    }

    Ui::SnapMacaroonDialog m_ui;
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    QPointer<MacaroonDialog> dialog = new MacaroonDialog;
    dialog->show();
    auto ret = app.exec();
    delete dialog;
    return ret;
}
