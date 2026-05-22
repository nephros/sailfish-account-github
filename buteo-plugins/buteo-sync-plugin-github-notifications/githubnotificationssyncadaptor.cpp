// SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
// SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
// SPDX-FileCopyrightText: 2026 2025,2026 Peter G. <sailfish@nephros.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "githubnotificationssyncadaptor.h"
#include "trace.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>

static const int NOTIFICATIONS_LIMIT = 30;

Q_LOGGING_CATEGORY(lcGithubNotifications, "buteo.plugin.github.notifications", QtWarningMsg)

GithubNotificationsSyncAdaptor::GithubNotificationsSyncAdaptor(QObject *parent)
    : GithubNotificationsDataTypeSyncAdaptor(SocialNetworkSyncAdaptor::Notifications, parent)
{
    setInitialActive(m_db.isValid());
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
    m_db.removeNotifications(oldId);
    m_db.sync();
    m_db.wait();
}

void GithubNotificationsSyncAdaptor::beginSync(int accountId, const QString &accessToken)
{
    requestNotifications(accountId, accessToken);
}

void GithubNotificationsSyncAdaptor::finalize(int accountId)
{
    if (syncAborted()) {
        qCDebug(lcGithubNotifications) << "sync aborted, skipping finalize of Github Notifications from account:" << accountId;
    } else {
        m_db.sync();
        m_db.wait();
        setLastSuccessfulSyncTime(accountId);
    }
    Q_UNUSED(accountId);
}

void GithubNotificationsSyncAdaptor::requestNotifications(int accountId, const QString &accessToken, const QString &until, const QString &pagingToken)
{
    // TODO: result paging
    Q_UNUSED(until);
    Q_UNUSED(pagingToken);

    QList<QPair<QString, QString> > queryItems;
    //queryItems.append(QPair<QString, QString>(QString(QLatin1String("all")), QString(QLatin1String("false"))));
    queryItems.append(QPair<QString, QString>(QString(QLatin1String("all")), QString(QLatin1String("true"))));
    queryItems.append(QPair<QString, QString>(QString(QLatin1String("participating")), QString(QLatin1String("true"))));
    QDateTime since = lastSuccessfulSyncTime(accountId);
    if (!since.isValid()) {
            int sinceSpan = m_accountSyncProfile
                    ? m_accountSyncProfile->key(Buteo::KEY_SYNC_SINCE_DAYS_PAST, QStringLiteral("7")).toInt()
                    : 7;
            since = QDateTime::currentDateTime().addDays(-1 * sinceSpan).toUTC();
    }
    //FIXME: format not accepted upstream...
    QString sincestr = since.toString(Qt::ISODate);
    qCInfo(lcGithubNotifications) << "setting since to" << sincestr;
    queryItems.append(QPair<QString, QString>(QString(QLatin1String("since")), sincestr));

    QUrl url(QStringLiteral("https://api.github.com/notifications"));
    QUrlQuery query(url);
    query.setQueryItems(queryItems);
    url.setQuery(query);

    QNetworkRequest request = QNetworkRequest(url);
    request.setRawHeader(QString(QLatin1String("Accept")).toUtf8(),
                    QString(QLatin1String("application/vnd.github+json")).toUtf8());
    request.setRawHeader(QString(QLatin1String("X-GitHub-Api-Version")).toUtf8(),
                    QString(QLatin1String("2022-11-28")).toUtf8());
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
                bool unread    = object.value(QStringLiteral("unread")).toBool();
                const QDateTime updated = QDateTime::fromString(object.value(QStringLiteral("updated_at")).toString(), Qt::ISODate);

                m_db.addGithubNotification(accountId,
                                           tid,
                                           type,
                                           title,
                                           from,
                                           reason,
                                           unread,
                                           repo,
                                           avatar,
                                           url,
                                           updated);
            } else {
                qCDebug(lcGithubNotifications) << "notification object empty; skipping";
            }
        }
        m_db.sync();
        m_db.wait();
    } else {
        qCWarning(lcGithubNotifications) << "unable to update notifications data for account" << accountId
                                  << ", got:" << QString::fromUtf8(replyData);
    }

    decrementSemaphore(accountId);
}

QDateTime GithubNotificationsSyncAdaptor::lastSuccessfulSyncTime(int accountId)
{
    QDateTime result;
    QString settingsFileName = QString::fromLatin1("%1/%2/ghnotif.ini")
            .arg(PRIVILEGED_DATA_DIR)
            .arg(SYNC_DATABASE_DIR);
    QSettings settingsFile(settingsFileName, QSettings::IniFormat);
    uint timestamp = settingsFile.value(QString::fromLatin1("%1-last-successful-sync-time").arg(accountId)).toUInt();
    if (timestamp > 0) {
        result = QDateTime::fromTime_t(timestamp);
    }
    return result;
}

void GithubNotificationsSyncAdaptor::setLastSuccessfulSyncTime(int accountId)
{
    QDateTime currentTime = QDateTime::currentDateTime().toUTC();
    QString settingsFileName = QString::fromLatin1("%1/%2/ghnotif.ini")
            .arg(PRIVILEGED_DATA_DIR)
            .arg(SYNC_DATABASE_DIR);
    QSettings settingsFile(settingsFileName, QSettings::IniFormat);
    settingsFile.setValue(QString::fromLatin1("%1-last-successful-sync-time").arg(accountId),
                          QVariant::fromValue<uint>(currentTime.toTime_t()));
    settingsFile.sync();
}
