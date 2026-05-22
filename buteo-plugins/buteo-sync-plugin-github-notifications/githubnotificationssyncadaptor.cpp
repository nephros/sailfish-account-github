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

    const QMap<QString, QString> reasonMap = {
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

/*
QString MastodonNotificationsSyncAdaptor::authServiceName() const
{
    return QStringLiteral("mastodon-microblog");
}
*/

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

/*
QString MastodonNotificationsSyncAdaptor::sanitizeContent(const QString &content)
{
    return MastodonTextUtils::sanitizeContent(content);
}

QDateTime MastodonNotificationsSyncAdaptor::parseTimestamp(const QString &timestampString)
{
    return MastodonTextUtils::parseTimestamp(timestampString);
}
*/

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

    // https://docs.github.com/en/rest/activity/notifications?apiVersion=2022-11-28
    if (!isError && ok && data.count() > 0) {

        foreach (const QJsonValue &entry, data) {
            const QJsonObject object = entry.toObject();
            if (!object.isEmpty()) {
                // NB: the spec has this as a string! Also, conversion from JSON->String->UInt is very finicky.
                quint32 tid      = object.value(QStringLiteral("id")).toString().toULong();
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
                const QString url      = subj.value(QStringLiteral("url")).toString();

                const QString reason = object.value(QStringLiteral("reason")).toString();
                //bool unread    = object.value(QStringLiteral("unread")).toBool();
                const QDateTime updated = QDateTime::fromString(object.value(QStringLiteral("updated_at")).toString(), Qt::ISODate);

                QString body = QString("%1: %2 %3").arg(type).arg(from).arg(reasonMap.value(reason));
                /*
                 *  "type": "Issue"
                 *  "reason": "subscribed",
                 *  "title": "Greetings"
                 *  "name": "Hello-World",
                 *  "full_name": "octocat/Hello-World",
                 */

                PendingNotification pendingNotification;
                pendingNotification.notificationId = tid;
                pendingNotification.previewSummary = QString("%1 (%2)").arg(title).arg(type);
                pendingNotification.previewBody = body;
                pendingNotification.summary = QString("%1 (%2)").arg(title).arg(type);
                pendingNotification.body = body;
                pendingNotification.subText = from;
                pendingNotification.icon = avatar;
                pendingNotification.link = url;
                pendingNotification.timestamp = updated;
                PendingSyncState state = m_pendingSyncStates.value(accountId);
                state.pendingNotifications.insert(pendingNotification.notificationId, pendingNotification);
            } else {
                qCDebug(lcGithubNotifications) << "notification object empty; skipping";
            }
        }
        PendingSyncState state = m_pendingSyncStates.value(accountId);

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

