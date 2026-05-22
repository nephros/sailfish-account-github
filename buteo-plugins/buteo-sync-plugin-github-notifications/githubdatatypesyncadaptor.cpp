// SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
// SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
// SPDX-FileCopyrightText: 2024-2026 Peter G. <sailfish@nephros.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "githubdatatypesyncadaptor.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QVariantMap>

//libsailfishkeyprovider
#include <sailfishkeyprovider.h>

// libaccounts-qt5
#include <Accounts/Manager>
#include <Accounts/Account>
#include <Accounts/Service>
#include <Accounts/AccountService>

// libsignon-qt5
#include <SignOn/Identity>
#include <SignOn/AuthSession>
#include <SignOn/SessionData>

Q_LOGGING_CATEGORY(lcGithubNotificationsSync, "buteo.plugin.github.notifications.sync", QtWarningMsg)

GithubNotificationsDataTypeSyncAdaptor::GithubNotificationsDataTypeSyncAdaptor(
        SocialNetworkSyncAdaptor::DataType dataType,
        QObject *parent)
    : SocialNetworkSyncAdaptor(QStringLiteral("github"), dataType, 0, parent)
{
}

GithubNotificationsDataTypeSyncAdaptor::~GithubNotificationsDataTypeSyncAdaptor()
{
}

void GithubNotificationsDataTypeSyncAdaptor::sync(const QString &dataTypeString, int accountId)
{
    if (dataTypeString != SocialNetworkSyncAdaptor::dataTypeName(m_dataType)) {
        qCWarning(lcGithubNotificationsSync) << "Github" << SocialNetworkSyncAdaptor::dataTypeName(m_dataType) <<
                          "sync adaptor was asked to sync" << dataTypeString;
        setStatus(SocialNetworkSyncAdaptor::Error);
        return;
    }

    if (clientId().isEmpty()) {
        qCWarning(lcGithubNotificationsSync) << "clientId could not be retrieved for GitHub account" << accountId;
        setStatus(SocialNetworkSyncAdaptor::Error);
        return;
    }

    setStatus(SocialNetworkSyncAdaptor::Busy);
    updateDataForAccount(accountId);
    qCDebug(lcGithubNotificationsSync) << "successfully triggered sync with profile:" << m_accountSyncProfile->name();
}

void GithubNotificationsDataTypeSyncAdaptor::updateDataForAccount(int accountId)
{
    Accounts::Account *account = Accounts::Account::fromId(m_accountManager, accountId, this);
    if (!account) {
        qCWarning(lcGithubNotificationsSync) << "existing account with id" << accountId << "couldn't be retrieved";
        setStatus(SocialNetworkSyncAdaptor::Error);
        return;
    }

    incrementSemaphore(accountId);
    signIn(account);
}

void GithubNotificationsDataTypeSyncAdaptor::errorHandler(QNetworkReply::NetworkError err)
{
        QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
        QByteArray replyData = reply->readAll();
        int accountId = reply->property("accountId").toInt();
        QString accessToken = reply->property("accessToken").toString();

        qCWarning(lcGithubNotificationsSync) << SocialNetworkSyncAdaptor::dataTypeName(m_dataType) <<
                "request with account" << accountId <<
                "experienced error:" << err <<
                "HTTP:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    // set "isError" on the reply so that adapters know to ignore the result in the finished() handler
    reply->setProperty("isError", QVariant::fromValue<bool>(true));
    // Note: not all errors are "unrecoverable" errors, so we don't change the status here.

    bool ok = false;
    QJsonObject parsed = parseJsonObjectReplyData(replyData, &ok);
    if (ok && parsed.contains(QLatin1String("message"))) {
        // FIXME: deal with errors correctly:
        // ... actually we might want to use HTTP status instead of parsing the response...

        QString errorMessage = parsed.value("message").toString();
        qCWarning(lcGithubNotificationsSync) << SocialNetworkSyncAdaptor::dataTypeName(m_dataType) << "reply message was: " << errorMessage ;
    }
}

void GithubNotificationsDataTypeSyncAdaptor::sslErrorsHandler(const QList<QSslError> &errs)
{
    QString sslerrs;
    foreach (const QSslError &e, errs) {
        sslerrs += e.errorString() + "; ";
    }
    if (errs.size() > 0) {
        sslerrs.chop(2);
    }
    qCWarning(lcGithubNotificationsSync) << SocialNetworkSyncAdaptor::dataTypeName(m_dataType) <<
                      "request with account" << sender()->property("accountId").toInt() <<
                      "experienced ssl errors:" << sslerrs;
    // set "isError" on the reply so that adapters know to ignore the result in the finished() handler
    sender()->setProperty("isError", QVariant::fromValue<bool>(true));
    // Note: not all errors are "unrecoverable" errors, so we don't change the status here.
}


QString GithubNotificationsDataTypeSyncAdaptor::clientId()
{
    if (!m_triedLoading) {
        loadClientIdAndSecret();
    }
    return m_clientId;
}

QString GithubNotificationsDataTypeSyncAdaptor::clientSecret()
{
    if (!m_triedLoading) {
        loadClientIdAndSecret();
    }
    return m_clientSecret;
}

void GithubNotificationsDataTypeSyncAdaptor::loadClientIdAndSecret()
{
    m_triedLoading = true;
    char *cClientId = NULL;
    char *cClientSecret = NULL;

    int cSuccess = SailfishKeyProvider_storedKey("github", "github-notifications", "client_id", &cClientId);
    if (cClientId == NULL) {
        return;
    } else if (cSuccess != 0) {
        free(cClientId);
        return;
    }

    m_clientId = QLatin1String(cClientId);
    free(cClientId);

    cSuccess = SailfishKeyProvider_storedKey("github", "github-notifications", "client_secret", &cClientSecret);
    if (cClientSecret == NULL) {
        return;
    } else if (cSuccess != 0) {
        free(cClientSecret);
        return;
    }

    m_clientSecret = QLatin1String(cClientSecret);
    free(cClientSecret);
}

void GithubNotificationsDataTypeSyncAdaptor::setCredentialsNeedUpdate(Accounts::Account *account)
{
    qCInfo(lcGithubNotificationsSync) << "sociald:Github: setting CredentialsNeedUpdate to true for account:" << account->id();
    Accounts::Service srv(m_accountManager->service(syncServiceName()));
    account->selectService(srv);
    account->setValue(QStringLiteral("CredentialsNeedUpdate"), QVariant::fromValue<bool>(true));
    account->setValue(QStringLiteral("CredentialsNeedUpdateFrom"), QVariant::fromValue<QString>(QString::fromLatin1("sociald-github")));
    account->selectService(Accounts::Service());
    account->syncAndBlock();
}

void GithubNotificationsDataTypeSyncAdaptor::signIn(Accounts::Account *account)
{
    const int accountId = account->id();
    if (!checkAccount(account) || clientId().isEmpty() || clientSecret().isEmpty()) {
        decrementSemaphore(accountId);
        return;
    }

    Accounts::Service srv(m_accountManager->service(syncServiceName()));
    account->selectService(srv);
    SignOn::Identity *identity = account->credentialsId() > 0 ? SignOn::Identity::existingIdentity(account->credentialsId()) : 0;
    if (!identity) {
        qCWarning(lcGithubNotificationsSync) << "error: account has no valid credentials, cannot sign in:" << accountId;
        decrementSemaphore(accountId);
        return;
    }

    Accounts::AccountService accSrv(account, srv);
    QString method = accSrv.authData().method();
    QString mechanism = accSrv.authData().mechanism();
    SignOn::AuthSession *session = identity->createSession(method);
    if (!session) {
        qCWarning(lcGithubNotificationsSync) << "error: could not create signon session for account:" << accountId;
        identity->deleteLater();
        decrementSemaphore(accountId);
        return;
    }

    QVariantMap signonSessionData = accSrv.authData().parameters();
    signonSessionData.insert("ClientId", clientId());
    signonSessionData.insert("ClientSecret", clientSecret());
    signonSessionData.insert("UiPolicy", SignOn::NoUserInteractionPolicy);

    connect(session, SIGNAL(response(SignOn::SessionData)),
            this, SLOT(signOnResponse(SignOn::SessionData)),
            Qt::UniqueConnection);
    connect(session, SIGNAL(error(SignOn::Error)),
            this, SLOT(signOnError(SignOn::Error)),
            Qt::UniqueConnection);

    session->setProperty("account", QVariant::fromValue<Accounts::Account*>(account));
    session->setProperty("identity", QVariant::fromValue<SignOn::Identity*>(identity));
    session->process(SignOn::SessionData(signonSessionData), mechanism);
}

void GithubNotificationsDataTypeSyncAdaptor::signOnError(const SignOn::Error &error)
{
    SignOn::AuthSession *session = qobject_cast<SignOn::AuthSession*>(sender());
    Accounts::Account *account = session->property("account").value<Accounts::Account*>();
    SignOn::Identity *identity = session->property("identity").value<SignOn::Identity*>();
    int accountId = account->id();
    qCWarning(lcGithubNotificationsSync) << "credentials for account with id" << accountId <<
                      "couldn't be retrieved:" << error.type() << "," << error.message();

    // if the error is because credentials have expired, we
    // set the CredentialsNeedUpdate key.
    if (error.type() == SignOn::Error::UserInteraction) {
        setCredentialsNeedUpdate(account);
    }

    session->disconnect(this);
    identity->destroySession(session);
    identity->deleteLater();
    account->deleteLater();

    // if we couldn't sign in, we can't sync with this account.
    setStatus(SocialNetworkSyncAdaptor::Error);
    decrementSemaphore(accountId);
}

void GithubNotificationsDataTypeSyncAdaptor::signOnResponse(const SignOn::SessionData &responseData)
{
    QVariantMap data;
    foreach (const QString &key, responseData.propertyNames()) {
        data.insert(key, responseData.getProperty(key));
    }

    QString accessToken;
    SignOn::AuthSession *session = qobject_cast<SignOn::AuthSession*>(sender());
    Accounts::Account *account = session->property("account").value<Accounts::Account*>();
    SignOn::Identity *identity = session->property("identity").value<SignOn::Identity*>();
    const int accountId = account->id();

    if (data.contains(QLatin1String("AccessToken"))) {
        accessToken = data.value(QLatin1String("AccessToken")).toString();
    } else {
        qCInfo(lcGithubNotificationsSync) << "signon response for account with id" << accountId << "contained no oauth token";
    }

    session->disconnect(this);
    identity->destroySession(session);
    identity->deleteLater();
    account->deleteLater();

    if (!accessToken.isEmpty()) {
        beginSync(accountId, accessToken);
    } else {
        setStatus(SocialNetworkSyncAdaptor::Error);
    }

    decrementSemaphore(accountId);
}
