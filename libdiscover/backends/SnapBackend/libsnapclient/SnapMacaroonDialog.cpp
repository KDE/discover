/***************************************************************************
 *   Copyright © 2017 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include <QApplication>
#include <QBuffer>
#include <QPointer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTextStream>
#include <KAuthExecuteJob>
#include <QDebug>
#include "ui_SnapMacaroonDialog.h"

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

    void login(const QString& username, const QString& password, const QString& otp = {})
    {
        KAuth::Action snapAction(QStringLiteral("org.kde.discover.libsnapclient.login"));
        snapAction.setHelperId(QStringLiteral("org.kde.discover.libsnapclient"));
        snapAction.setArguments({
            { QStringLiteral("user"), username },
            { QStringLiteral("password"), password },
            { QStringLiteral("otp"), otp }
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

    void replied(KJob* job)
    {
        KAuth::ExecuteJob* reply = static_cast<KAuth::ExecuteJob*>(job);
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


int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    QPointer<MacaroonDialog> dialog = new MacaroonDialog;
    dialog->show();
    auto ret = app.exec();
    delete dialog;
    return ret;
}
