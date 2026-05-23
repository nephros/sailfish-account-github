// SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
// SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
// SPDX-FileCopyrightText: 2026 2025,2026 Peter G. <sailfish@nephros.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "githubnotificationssyncadaptor.h"
#include "trace.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QLoggingCategory>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>
#include <QtNetwork/QNetworkRequest>

// libaccounts-qt5
#include <Accounts/Account>
#include <Accounts/Manager>
#include <Accounts/Service>

#include <notification.h>

#define GITHUB_API_VERSION "2022-11-28"

#define OPEN_URL_ACTION(openUrl)            \
    Notification::remoteAction(             \
        "default",                          \
        "",                                 \
        "org.sailfishos.fileservice",       \
        "/",                                \
        "org.sailfishos.fileservice",       \
        "openUrl",                          \
        QVariantList() << openUrl           \
    )

namespace {
    Q_LOGGING_CATEGORY(lcGithubNotifications, "buteo.plugin.github.notifications", QtWarningMsg)

    const char *const NotificationCategory = "x-nemo.social.github.notification";
    const char *const NotificationIdHint = "x-nemo.sociald.notification-id";
    const char *const LastFetchedNotificationIdKey = "LastFetchedNotificationId";
    const int NotificationsPageLimit = 80;
    const uint NotificationDismissedReason = 1;

    bool hasActiveNotificationsForAccount(int accountId, const Notification *ignoredNotification = 0)
    {
        bool hasActiveNotifications = false;
        const QList<QObject *> notifications = Notification::notifications();
        foreach (QObject *object, notifications) {
            Notification *notification = qobject_cast<Notification *>(object);
            if (notification
                    && notification != ignoredNotification
                    && notification->category() == QLatin1String(NotificationCategory)
                    && notification->hintValue("x-nemo.sociald.account-id").toInt() == accountId) {
                hasActiveNotifications = true;
            }

            delete object;
        }

        return hasActiveNotifications;
    }

    //% "GitHub"
    const char *const TrIdGitHub = QT_TRID_NOOP("lipstick-jolla-home-la-github-notification-github");
    //% "New notification"
    const char *const TrIdNewNotification = QT_TRID_NOOP("lipstick-jolla-home-la-github-notification-new_notification");

    //% "You were assigned to the issue."
    const char *const TrIdReasonAssign = QT_TRID_NOOP("lipstick-jolla-home-la-github-notification-reason_assign");
    //% "You created the thread."
    const char *const TrIdReasonAuthor = QT_TRID_NOOP("lipstick-jolla-home-la-github-notification-reason_author");
    //% "You commented on the thread."
    const char *const TrIdReasonComment = QT_TRID_NOOP("lipstick-jolla-home-la-github-notification-reason_comment");
    //% "A GitHub Actions workflow run that you triggered was completed."
    const char *const TrIdReasonCI = QT_TRID_NOOP("lipstick-jolla-home-la-github-notification-reason_ci_activity");
    //% "You accepted an invitation to contribute to the repository."
    const char *const TrIdReasonInvitation = QT_TRID_NOOP("lipstick-jolla-home-la-github-notification-reason_invitation");
    //% "You subscribed to the thread (via an issue or pull request)."
    const char *const TrIdReasonManual = QT_TRID_NOOP("lipstick-jolla-home-la-github-notification-reason_manual");
    //% "You were specifically @mentioned in the content."
    const char *const TrIdReasonMention = QT_TRID_NOOP("lipstick-jolla-home-la-github-notification-reason_mention");
    //% "You, or a team you're a member of, were requested to review a pull request."
    const char *const TrIdReasonReview = QT_TRID_NOOP("lipstick-jolla-home-la-github-notification-reason_review");
    //% "GitHub discovered a security vulnerability in your repository."
    const char *const TrIdReasonAlert = QT_TRID_NOOP("lipstick-jolla-home-la-github-notification-reason_alert");
    //% "You changed the thread state (for example, closing an issue or merging a pull request)."
    const char *const TrIdReasonStateChange = QT_TRID_NOOP("lipstick-jolla-home-la-github-notification-reason_state_change");
    //% "You're watching the repository."
    const char *const TrIdReasonSubscribe = QT_TRID_NOOP("lipstick-jolla-home-la-github-notification-reason_subscribed");
    //% "You were on a team that was mentioned."
    const char *const TrIdReasonTeamMention = QT_TRID_NOOP("lipstick-jolla-home-la-github-notification-reason_team_mention");

    const QMap<QString, QString> reasonStringMap = {
        { QStringLiteral("assign"), TrIdReasonAssign },
        { QStringLiteral("author"), TrIdReasonAuthor },
        { QStringLiteral("comment"), TrIdReasonComment },
        { QStringLiteral("ci_activity"), TrIdReasonCI },
        { QStringLiteral("invitation"), TrIdReasonInvitation },
        { QStringLiteral("manual"), TrIdReasonManual },
        { QStringLiteral("mention"), TrIdReasonMention },
        { QStringLiteral("review_requested"), TrIdReasonReview },
        { QStringLiteral("security_alert"), TrIdReasonAlert },
        { QStringLiteral("state_change"), TrIdReasonStateChange },
        { QStringLiteral("subscribed"), TrIdReasonSubscribe },
        { QStringLiteral("team_mention"), TrIdReasonTeamMention }
    };
    const QMap<QString, QString> reasonJiMap = {
        { QStringLiteral("assign"), QStringLiteral("🎯") },
        { QStringLiteral("author"), "" },
        { QStringLiteral("comment"), QStringLiteral("💬") },
        { QStringLiteral("ci_activity"), "" },
        { QStringLiteral("invitation"), QStringLiteral("📨") },
        { QStringLiteral("manual"), ""  },
        { QStringLiteral("mention"), QStringLiteral("✋") },
        { QStringLiteral("review_requested"), QStringLiteral("👀") },
        { QStringLiteral("security_alert"), QStringLiteral("_") },
        { QStringLiteral("state_change"), "" },
        { QStringLiteral("subscribed"), QStringLiteral("👁") },
        { QStringLiteral("team_mention"), QStringLiteral("🙌") }
    };

    const QMap<QString, QString> typeIconMap = {
        // see https://primer.style/design/foundations/icons
       { QStringLiteral("Action"),           QStringLiteral("data:image/svg+xml;utf8,<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 16 16\" width=\"16\" height=\"16\"><path fill=\"darkgray\"    d=\"M8 0a8 8 0 1 1 0 16A8 8 0 0 1 8 0ZM1.5 8a6.5 6.5 0 1 0 13 0 6.5 6.5 0 0 0-13 0Zm4.879-2.773 4.264 2.559a.25.25 0 0 1 0 .428l-4.264 2.559A.25.25 0 0 1 6 10.559V5.442a.25.25 0 0 1 .379-.215Z\"></path></svg>") },
       { QStringLiteral("Issue"),            QStringLiteral("data:image/svg+xml;utf8,<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 16 16\" width=\"16\" height=\"16\"><path fill=\"darkgreen\"   d=\"M8 9.5a1.5 1.5 0 1 0 0-3 1.5 1.5 0 0 0 0 3Z\"></path><path fill=\"darkgreen\" d=\"M8 0a8 8 0 1 1 0 16A8 8 0 0 1 8 0ZM1.5 8a6.5 6.5 0 1 0 13 0 6.5 6.5 0 0 0-13 0Z\"></path></svg>") },
       { QStringLiteral("IssueClosed"),      QStringLiteral("data:image/svg+xml;utf8,<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 16 16\" width=\"16\" height=\"16\"><path fill=\"darkmagenta\" d=\"M11.28 6.78a.75.75 0 0 0-1.06-1.06L7.25 8.69 5.78 7.22a.75.75 0 0 0-1.06 1.06l2 2a.75.75 0 0 0 1.06 0l3.5-3.5Z\"></path><path d=\"M16 8A8 8 0 1 1 0 8a8 8 0 0 1 16 0Zm-1.5 0a6.5 6.5 0 1 0-13 0 6.5 6.5 0 0 0 13 0Z\"></path></svg>") },
       { QStringLiteral("IssueReopened"),    QStringLiteral("data:image/svg+xml;utf8,<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 16 16\" width=\"16\" height=\"16\"><path fill=\"darkgreen\"   d=\"M5.029 2.217a6.5 6.5 0 0 1 9.437 5.11.75.75 0 1 0 1.492-.154 8 8 0 0 0-14.315-4.03L.427 1.927A.25.25 0 0 0 0 2.104V5.75A.25.25 0 0 0 .25 6h3.646a.25.25 0 0 0 .177-.427L2.715 4.215a6.491 6.491 0 0 1 2.314-1.998ZM1.262 8.169a.75.75 0 0 0-1.22.658 8.001 8.001 0 0 0 14.315 4.03l1.216 1.216a.25.25 0 0 0 .427-.177V10.25a.25.25 0 0 0-.25-.25h-3.646a.25.25 0 0 0-.177.427l1.358 1.358a6.501 6.501 0 0 1-11.751-3.11.75.75 0 0 0-.272-.506Z\"></path><path d=\"M9.06 9.06a1.5 1.5 0 1 1-2.12-2.12 1.5 1.5 0 0 1 2.12 2.12Z\"></path></svg>") },
       { QStringLiteral("Merge"),            QStringLiteral("data:image/svg+xml;utf8,<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 16 16\" width=\"16\" height=\"16\"><path fill=\"darkmagenta\" d=\"M5.45 5.154A4.25 4.25 0 0 0 9.25 7.5h1.378a2.251 2.251 0 1 1 0 1.5H9.25A5.734 5.734 0 0 1 5 7.123v3.505a2.25 2.25 0 1 1-1.5 0V5.372a2.25 2.25 0 1 1 1.95-.218ZM4.25 13.5a.75.75 0 1 0 0-1.5.75.75 0 0 0 0 1.5Zm8.5-4.5a.75.75 0 1 0 0-1.5.75.75 0 0 0 0 1.5ZM5 3.25a.75.75 0 1 0 0 .005V3.25Z\"></path></svg>") },
       { QStringLiteral("PullRequest"),      QStringLiteral("data:image/svg+xml;utf8,<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 16 16\" width=\"16\" height=\"16\"><path fill=\"darkgreen\"   d=\"M1.5 3.25a2.25 2.25 0 1 1 3 2.122v5.256a2.251 2.251 0 1 1-1.5 0V5.372A2.25 2.25 0 0 1 1.5 3.25Zm5.677-.177L9.573.677A.25.25 0 0 1 10 .854V2.5h1A2.5 2.5 0 0 1 13.5 5v5.628a2.251 2.251 0 1 1-1.5 0V5a1 1 0 0 0-1-1h-1v1.646a.25.25 0 0 1-.427.177L7.177 3.427a.25.25 0 0 1 0-.354ZM3.75 2.5a.75.75 0 1 0 0 1.5.75.75 0 0 0 0-1.5Zm0 9.5a.75.75 0 1 0 0 1.5.75.75 0 0 0 0-1.5Zm8.25.75a.75.75 0 1 0 1.5 0 .75.75 0 0 0-1.5 0Z\"></path></svg>") },
       { QStringLiteral("PullRequestClosed"),QStringLiteral("data:image/svg+xml;utf8,<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 16 16\" width=\"16\" height=\"16\"><path fill=\"crimson\"     d=\"M3.25 1A2.25 2.25 0 0 1 4 5.372v5.256a2.251 2.251 0 1 1-1.5 0V5.372A2.251 2.251 0 0 1 3.25 1Zm9.5 5.5a.75.75 0 0 1 .75.75v3.378a2.251 2.251 0 1 1-1.5 0V7.25a.75.75 0 0 1 .75-.75Zm-2.03-5.273a.75.75 0 0 1 1.06 0l.97.97.97-.97a.748.748 0 0 1 1.265.332.75.75 0 0 1-.205.729l-.97.97.97.97a.751.751 0 0 1-.018 1.042.751.751 0 0 1-1.042.018l-.97-.97-.97.97a.749.749 0 0 1-1.275-.326.749.749 0 0 1 .215-.734l.97-.97-.97-.97a.75.75 0 0 1 0-1.06ZM2.5 3.25a.75.75 0 1 0 1.5 0 .75.75 0 0 0-1.5 0ZM3.25 12a.75.75 0 1 0 0 1.5.75.75 0 0 0 0-1.5Zm9.5 0a.75.75 0 1 0 0 1.5.75.75 0 0 0 0-1.5Z\"></path></svg>") },
       { QStringLiteral("PullRequestDraft"), QStringLiteral("data:image/svg+xml;utf8,<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 16 16\" width=\"16\" height=\"16\"><path fill=\"darkgray\"    d=\"M4.75 1.5a3.25 3.25 0 0 1 .745 6.414A.827.827 0 0 1 5.5 8v8a.827.827 0 0 1-.005.086A3.25 3.25 0 0 1 4.75 22.5a3.25 3.25 0 0 1-.745-6.414A.827.827 0 0 1 4 16V8c0-.029.002-.057.005-.086A3.25 3.25 0 0 1 4.75 1.5ZM16 19.25a3.25 3.25 0 1 1 6.5 0 3.25 3.25 0 0 1-6.5 0ZM3 4.75a1.75 1.75 0 1 0 3.501-.001A1.75 1.75 0 0 0 3 4.75Zm0 14.5a1.75 1.75 0 1 0 3.501-.001A1.75 1.75 0 0 0 3 19.25Zm16.25-1.75a1.75 1.75 0 1 0 .001 3.501 1.75 1.75 0 0 0-.001-3.501Zm0-11.5a1.75 1.75 0 1 0 0-3.5 1.75 1.75 0 0 0 0 3.5ZM21 11.25a1.75 1.75 0 1 1-3.5 0 1.75 1.75 0 0 1 3.5 0Z\"></path></svg>") },
       { QStringLiteral("SecurityAlert"),    QStringLiteral("data:image/svg+xml;utf8,<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 16 16\" width=\"16\" height=\"16\"><path fill=\"darkorange\"  d=\"M7.467.133a1.748 1.748 0 0 1 1.066 0l5.25 1.68A1.75 1.75 0 0 1 15 3.48V7c0 1.566-.32 3.182-1.303 4.682-.983 1.498-2.585 2.813-5.032 3.855a1.697 1.697 0 0 1-1.33 0c-2.447-1.042-4.049-2.357-5.032-3.855C1.32 10.182 1 8.566 1 7V3.48a1.75 1.75 0 0 1 1.217-1.667Zm.61 1.429a.25.25 0 0 0-.153 0l-5.25 1.68a.25.25 0 0 0-.174.238V7c0 1.358.275 2.666 1.057 3.86.784 1.194 2.121 2.34 4.366 3.297a.196.196 0 0 0 .154 0c2.245-.956 3.582-2.104 4.366-3.298C13.225 9.666 13.5 8.36 13.5 7V3.48a.251.251 0 0 0-.174-.237l-5.25-1.68ZM8.75 4.75v3a.75.75 0 0 1-1.5 0v-3a.75.75 0 0 1 1.5 0ZM9 10.5a1 1 0 1 1-2 0 1 1 0 0 1 2 0Z\"></path></svg>") },
       { QStringLiteral("Tag"),              QStringLiteral("data:image/svg+xml;utf8,<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 16 16\" width=\"16\" height=\"16\"><path fill=\"dimgray\"     d=\"M1 7.775V2.75C1 1.784 1.784 1 2.75 1h5.025c.464 0 .91.184 1.238.513l6.25 6.25a1.75 1.75 0 0 1 0 2.474l-5.026 5.026a1.75 1.75 0 0 1-2.474 0l-6.25-6.25A1.752 1.752 0 0 1 1 7.775Zm1.5 0c0 .066.026.13.073.177l6.25 6.25a.25.25 0 0 0 .354 0l5.025-5.025a.25.25 0 0 0 0-.354l-6.25-6.25a.25.25 0 0 0-.177-.073H2.75a.25.25 0 0 0-.25.25ZM6 5a1 1 0 1 1 0 2 1 1 0 0 1 0-2Z\"></path></svg>") },
       // is this a real type? Meh.
       { QStringLiteral("Repository"),       QStringLiteral("data:image/svg+xml;utf8,<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 16 16\" width=\"16\" height=\"16\"><path fill=\"dimgray\"     d=\"M2 2.5A2.5 2.5 0 0 1 4.5 0h8.75a.75.75 0 0 1 .75.75v12.5a.75.75 0 0 1-.75.75h-2.5a.75.75 0 0 1 0-1.5h1.75v-2h-8a1 1 0 0 0-.714 1.7.75.75 0 1 1-1.072 1.05A2.495 2.495 0 0 1 2 11.5Zm10.5-1h-8a1 1 0 0 0-1 1v6.708A2.486 2.486 0 0 1 4.5 9h8ZM5 12.25a.25.25 0 0 1 .25-.25h3.5a.25.25 0 0 1 .25.25v3.25a.25.25 0 0 1-.4.2l-1.45-1.087a.249.249 0 0 0-.3 0L5.4 15.7a.25.25 0 0 1-.4-.2Z\"></path></svg>") }
    };
}

GithubNotificationsSyncAdaptor::GithubNotificationsSyncAdaptor(QObject *parent)
    : GithubNotificationsDataTypeSyncAdaptor(SocialNetworkSyncAdaptor::Notifications, parent)
{
    setInitialActive(true);
}

GithubNotificationsSyncAdaptor::~GithubNotificationsSyncAdaptor()
{
}

QString GithubNotificationsSyncAdaptor::syncServiceName() const
{
    return QStringLiteral("github-notifications");
}

void GithubNotificationsSyncAdaptor::purgeDataForOldAccount(int oldId, SocialNetworkSyncAdaptor::PurgeMode)
{
    closeAccountNotifications(oldId);

    m_accessTokens.remove(oldId);
    m_pendingSyncStates.remove(oldId);
    m_lastMarkedReadIds.remove(oldId);
    saveLastFetchedId(oldId, QString());
}

void GithubNotificationsSyncAdaptor::beginSync(int accountId, const QString &accessToken)
{
    m_accessTokens.insert(accountId, accessToken);
    m_pendingSyncStates.remove(accountId);
    requestNotifications(accountId, accessToken);
}

void GithubNotificationsSyncAdaptor::finalize(int accountId)
{
    if (syncAborted()) {
        qCDebug(lcGithubNotifications) << "sync aborted, skipping finalize of Github Notifications from account:" << accountId;
    }

    Q_UNUSED(accountId);
}

int GithubNotificationsSyncAdaptor::compareNotificationIds(const QString &left, const QString &right)
{
    if (left == right) {
        return 0;
    }

    bool leftOk = false;
    bool rightOk = false;
    const qulonglong leftValue = left.toULongLong(&leftOk);
    const qulonglong rightValue = right.toULongLong(&rightOk);
    if (leftOk && rightOk) {
        return leftValue < rightValue ? -1 : 1;
    }

    if (left.size() != right.size()) {
        return left.size() < right.size() ? -1 : 1;
    }
    return left < right ? -1 : 1;
}

QString GithubNotificationsSyncAdaptor::notificationObjectKey(int accountId, const QString &notificationId)
{
    return QString::number(accountId) + QLatin1Char(':') + notificationId;
}

QString GithubNotificationsSyncAdaptor::loadLastFetchedId(int accountId) const
{
    Accounts::Account *account = Accounts::Account::fromId(m_accountManager, accountId, 0);
    if (!account) {
        return QString();
    }

    Accounts::Service service(m_accountManager->service(syncServiceName()));
    account->selectService(service);
    const QString lastFetchedId = account->value(QString::fromLatin1(LastFetchedNotificationIdKey)).toString().trimmed();
    account->deleteLater();

    return lastFetchedId;
}

void GithubNotificationsSyncAdaptor::saveLastFetchedId(int accountId, const QString &lastFetchedId)
{
    Accounts::Account *account = Accounts::Account::fromId(m_accountManager, accountId, 0);
    if (!account) {
        return;
    }

    Accounts::Service service(m_accountManager->service(syncServiceName()));
    account->selectService(service);
    const QString storedId = account->value(QString::fromLatin1(LastFetchedNotificationIdKey)).toString().trimmed();
    if (storedId != lastFetchedId) {
        account->setValue(QString::fromLatin1(LastFetchedNotificationIdKey), lastFetchedId);
        account->syncAndBlock();
    }

    account->deleteLater();
}





//void GithubNotificationsSyncAdaptor::requestNotifications(int accountId, const QString &accessToken, const QString &until, const QString &pagingToken)
void GithubNotificationsSyncAdaptor::requestNotifications(int accountId, const QString &accessToken)
{
    QList<QPair<QString, QString> > queryItems;
    queryItems.append(QPair<QString, QString>(QString(QLatin1String("all")), QString(QLatin1String("true"))));
    queryItems.append(QPair<QString, QString>(QString(QLatin1String("participating")), QString(QLatin1String("true"))));
    QUrl url(QStringLiteral("https://api.github.com/notifications"));
    QUrlQuery query(url);
    query.setQueryItems(queryItems);
    url.setQuery(query);

    QNetworkRequest request = QNetworkRequest(url);
    request.setRawHeader(QString(QLatin1String("Accept")).toUtf8(),
                    QString(QLatin1String("application/vnd.github+json")).toUtf8());
    request.setRawHeader(QString(QLatin1String("X-GitHub-Api-Version")).toUtf8(),
                    QString(QLatin1String(GITHUB_API_VERSION)).toUtf8());
    request.setRawHeader(QString(QLatin1String("Authorization")).toUtf8(),
                    QString(QLatin1String("Bearer ")).toUtf8() + accessToken.toUtf8());
    QNetworkReply *reply = m_networkAccessManager->get(request);

    if (reply) {
        reply->setProperty("accountId", accountId);
        reply->setProperty("accessToken", accessToken);
        connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(errorHandler(QNetworkReply::NetworkError)));
        connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrorsHandler(QList<QSslError>)));
        connect(reply, SIGNAL(finished()), this, SLOT(finishedNotificationsHandler()));

        incrementSemaphore(accountId);
        setupReplyTimeout(accountId, reply);
    } else {
        qCWarning(lcGithubNotifications) << "unable to request notifications from Github account with id" << accountId;
        decrementSemaphore(accountId);
    }
}

void GithubNotificationsSyncAdaptor::finishedNotificationsHandler()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }

    const bool isError = reply->property("isError").toBool();
    const int accountId = reply->property("accountId").toInt();
    const QByteArray replyData = reply->readAll();

    disconnect(reply);
    reply->deleteLater();
    removeReplyTimeout(accountId, reply);

    bool ok = false;
    const QJsonArray data = parseJsonArrayReplyData(replyData, &ok);
    PendingSyncState state = m_pendingSyncStates.value(accountId);

    // https://docs.github.com/en/rest/activity/notifications?apiVersion=2022-11-28
    if (!isError && ok && data.count() > 0) {
    qCDebug(lcGithubNotifications) << "got notifications:" << data.count();

        foreach (const QJsonValue &entry, data) {
            const QJsonObject object = entry.toObject();
            if (!object.isEmpty()) {
                // NB: the spec has this as a string! Also, conversion from JSON->String->UInt is very finicky.
                const quint32 tid      = object.value(QStringLiteral("id")).toString().toULong();
                if (tid == 0) {
                        qCWarning(lcGithubNotifications) << "Error: id is zero, (wither not a number or conversion failed) skipping entry.";
                        continue;
                }
                // repo data:
                const QJsonObject r    = object.value(QStringLiteral("repository")).toObject();
                const QString from     = r.value(QStringLiteral("full_name")).toString();
                const QString repo     = QLatin1String(QJsonDocument(r).toJson(QJsonDocument::Compact)); // full metadata
                // Owner data
                const QJsonObject ow   = r.value(QStringLiteral("owner")).toObject();
                const QString avatar   = ow.value(QStringLiteral("avatar_url")).toString();

                // subject/message
                const QJsonObject subj = object.value(QStringLiteral("subject")).toObject();
                const QString title    = subj.value(QStringLiteral("title")).toString();
                const QString type     = subj.value(QStringLiteral("type")).toString();
                // which URL to use?
                // we have:
                //  - .url --> go to notification thread
                //  - .subject.url --> go to subject, e.g. URL of an issue
                //  - .subject.latest_comment_url --> at least for issues...
                //  - .repository.url --> URL of repo which "sent" this notification
                //  - .repository.owner.html_url --> URL of user who "sent" this notification
                //const QString url      = subj.value(QStringLiteral("url")).toString();
                QUrl url(subj.value(QStringLiteral("url")).toString());
                // url has api.github.com as the host, replace that:
                url.setHost(QStringLiteral("github.com"));

                const QString reason = object.value(QStringLiteral("reason")).toString();
                //bool unread    = object.value(QStringLiteral("unread")).toBool();
                const QDateTime updated = QDateTime::fromString(object.value(QStringLiteral("updated_at")).toString(), Qt::ISODate);

                /*
                 *  "type": "Issue"
                 *  "reason": "subscribed",
                 *  "title": "Greetings"
                 *  "name": "Hello-World",
                 *  "full_name": "octocat/Hello-World",
                 */

                PendingNotification pendingNotification;
                pendingNotification.notificationId = QString::number(tid);
                //pendingNotification.previewSummary = QString("%1 (%2)").arg(title).arg(type);
                //pendingNotification.previewSummary = QString("%1 (%2)").arg(from).arg(type);
                //pendingNotification.previewBody = title;
                //pendingNotification.summary = QString("%1 (%2)").arg(title).arg(type);
                pendingNotification.summary = QString("%1 (%2)").arg(from).arg(type);
                pendingNotification.body = title;
                pendingNotification.subText = reasonJiMap.value(reason);
                //pendingNotification.icon = avatar;
                pendingNotification.icon = typeIconMap.value(type);
                pendingNotification.link = url.toString();
                pendingNotification.timestamp = updated;
                state.pendingNotifications.insert( QString::number(tid), pendingNotification);
            } else {
                qCDebug(lcGithubNotifications) << "notification object empty; skipping";
            }
        }

        if (state.pendingNotifications.size() > 0) {
            QStringList notificationIds = state.pendingNotifications.keys();
            std::sort(notificationIds.begin(), notificationIds.end(), [](const QString &left, const QString &right) {
                return compareNotificationIds(left, right) > 0;
            });

            foreach (const QString &notificationId, notificationIds) {
                const PendingNotification pendingNotification = state.pendingNotifications.value(notificationId);
                publishSystemNotification(accountId, pendingNotification);
            }
        }

    } else {
        qCWarning(lcGithubNotifications) << "unable to update notifications data for account" << accountId
                                  << ", got:" << QString::fromUtf8(replyData);
    }

    decrementSemaphore(accountId);
}

void GithubNotificationsSyncAdaptor::publishSystemNotification(int accountId,
                                                               const PendingNotification &notificationData)
{
    Notification *notification = createNotification(accountId, notificationData.notificationId);
    notification->setItemCount(1);
    notification->setTimestamp(notificationData.timestamp.isValid()
                               ? notificationData.timestamp
                               : QDateTime::currentDateTimeUtc());
    notification->setSummary(notificationData.summary.isEmpty()
                             ? qtTrId(TrIdGitHub)
                             : notificationData.summary);
    notification->setBody(notificationData.body.isEmpty()
                          ? qtTrId(TrIdNewNotification)
                          : notificationData.body);
    notification->setPreviewSummary(notificationData.summary);
    notification->setPreviewBody(notificationData.body);
    notification->setIcon(notificationData.icon);

    const QString openUrl = notificationData.link.isEmpty()
            ? QStringLiteral("https://github.com//notifications")
            : notificationData.link;
    const QUrl parsedOpenUrl(openUrl);
    notification->setRemoteAction(OPEN_URL_ACTION(openUrl));
    notification->publish();
    if (notification->replacesId() == 0) {
        qCWarning(lcGithubNotifications) << "failed to publish GitHub notification"
                                  << notificationData.notificationId;
    }
}

void GithubNotificationsSyncAdaptor::notificationClosedWithReason(uint reason)
{
    Notification *notification = qobject_cast<Notification *>(sender());
    removeCachedNotification(notification);
    if (reason == NotificationDismissedReason) {
        markReadFromNotification(notification);
    }
}

void GithubNotificationsSyncAdaptor::maybeMarkAccountNotificationsRead(int accountId,
                                                                         const QString &accessToken,
                                                                         Notification *ignoredNotification)
{
    if (accountId <= 0 || accessToken.isEmpty()) {
        return;
    }

    if (hasActiveNotificationsForAccount(accountId, ignoredNotification)) {
        return;
    }

    const QString lastReadId = loadLastFetchedId(accountId);
    if (lastReadId.isEmpty()) {
        return;
    }

    const QString currentMarkerId = m_lastMarkedReadIds.value(accountId);
    if (!currentMarkerId.isEmpty() && compareNotificationIds(lastReadId, currentMarkerId) <= 0) {
        return;
    }

    requestMarkRead(accountId, accessToken, lastReadId);
}

void GithubNotificationsSyncAdaptor::markReadFromNotification(Notification *notification)
{
    if (!notification) {
        return;
    }

    const int accountId = notification->hintValue("x-nemo.sociald.account-id").toInt();
    const QString accessToken = m_accessTokens.value(accountId).trimmed();
    if (accountId <= 0 || accessToken.isEmpty()) {
        return;
    }

    maybeMarkAccountNotificationsRead(accountId, accessToken, notification);
}

void GithubNotificationsSyncAdaptor::removeCachedNotification(Notification *notification)
{
    if (!notification) {
        return;
    }

    const int accountId = notification->hintValue("x-nemo.sociald.account-id").toInt();
    const QString notificationId = notification->hintValue(NotificationIdHint).toString();
    if (accountId <= 0 || notificationId.isEmpty()) {
        return;
    }

    m_notificationObjects.remove(notificationObjectKey(accountId, notificationId));
}

Notification *GithubNotificationsSyncAdaptor::createNotification(int accountId, const QString &notificationId)
{
    const QString objectKey = notificationObjectKey(accountId, notificationId);
    Notification *notification = m_notificationObjects.value(objectKey);
    if (!notification) {
        notification = findNotification(accountId, notificationId);
    }
    if (!notification) {
        notification = new Notification(this);
    } else if (notification->parent() != this) {
        notification->setParent(this);
    }

    notification->setAppName(QStringLiteral("GitHub"));
    notification->setAppIcon(QStringLiteral("github-mark-white"));
    notification->setHintValue("x-nemo.sociald.account-id", accountId);
    notification->setHintValue(NotificationIdHint, notificationId);
    notification->setHintValue("x-nemo-feedback", QStringLiteral("social"));
    //notification->setHintValue("x-nemo-priority", 100); // Show on lockscreen
    notification->setCategory(QLatin1String(NotificationCategory));

    connect(notification, SIGNAL(closed(uint)), this, SLOT(notificationClosedWithReason(uint)), Qt::UniqueConnection);
    m_notificationObjects.insert(objectKey, notification);

    return notification;
}

Notification *GithubNotificationsSyncAdaptor::findNotification(int accountId, const QString &notificationId)
{
    Notification *notification = 0;
    QList<QObject *> notifications = Notification::notifications();
    foreach (QObject *object, notifications) {
        Notification *castedNotification = qobject_cast<Notification *>(object);
        if (castedNotification
                && castedNotification->category() == QLatin1String(NotificationCategory)
                && castedNotification->hintValue("x-nemo.sociald.account-id").toInt() == accountId
                && castedNotification->hintValue(NotificationIdHint).toString() == notificationId) {
            notification = castedNotification;
            break;
        }
    }

    if (notification) {
        notifications.removeAll(notification);
    }

    qDeleteAll(notifications);

    return notification;
}
