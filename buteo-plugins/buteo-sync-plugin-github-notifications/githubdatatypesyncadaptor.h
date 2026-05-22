/*
 * SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-FileCopyrightText: 2026 2026 Jolla Mobile Ltd
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef GITHUBNOTIFICATIONSDATATYPESYNCADAPTOR_H
#define GITHUBNOTIFICATIONSDATATYPESYNCADAPTOR_H

#include "socialnetworksyncadaptor.h"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QSslError>
#include <QtCore/QJsonObject>

namespace Accounts {
    class Account;
}
namespace SignOn {
    class Error;
    class SessionData;
}

class GithubNotificationsDataTypeSyncAdaptor : public SocialNetworkSyncAdaptor
{
    Q_OBJECT

public:
    GithubNotificationsDataTypeSyncAdaptor(SocialNetworkSyncAdaptor::DataType dataType, QObject *parent);
    virtual ~GithubNotificationsDataTypeSyncAdaptor();

    void sync(const QString &dataTypeString, int accountId);

protected:
    QString clientId();
    QString clientSecret();

    virtual void updateDataForAccount(int accountId);
    virtual QString authServiceName() const;
    virtual void beginSync(int accountId, const QString &accessToken) = 0;

protected Q_SLOTS:
    virtual void errorHandler(QNetworkReply::NetworkError err);
    virtual void sslErrorsHandler(const QList<QSslError> &errs);

private Q_SLOTS:
    void signOnError(const SignOn::Error &error);
    void signOnResponse(const SignOn::SessionData &responseData);

private:
    void setCredentialsNeedUpdate(Accounts::Account *account);
    void signIn(Accounts::Account *account);

    void loadClientIdAndSecret();
    bool m_triedLoading; // Is true if we tried to load (even if we failed)

    QString m_clientId;
    QString m_clientSecret;
};

#endif // GITHUBNOTIFICATIONSDATATYPESYNCADAPTOR_H
