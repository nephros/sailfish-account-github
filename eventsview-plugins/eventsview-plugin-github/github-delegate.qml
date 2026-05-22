/****************************************************************************
 **
 ** Copyright (C) 2014 - 2015 Jolla Ltd.
 ** Copyright (C) 2020 Open Mobile Platform LLC.
 ** Copyright (C) 2023 Peter G.
 **
 ****************************************************************************/

import QtQuick 2.0
import Sailfish.Silica 1.0
import org.nemomobile.social 1.0
import org.nemomobile.socialcache 1.0
import "shared"

SocialMediaAccountDelegate {
    id: delegateItem

    //% "GitHub Notifications"
    headerText: qsTrId("lipstick-jolla-home-la-github_notifications")
    headerIcon: ( Theme.colorScheme === Theme.LightOnDark )
              ? "image://theme/github-mark-white"
              : "image://theme/github-mark"

    services: [ "Notifications"]
    /*
     * FIXME: should correspond to enum at https://github.com/sailfishos/libsocialcache/blob/e59e66214d174d699ab2fa3e41976add83da9e7e/src/lib/socialsyncinterface.h#L32
     * socialsyncinterface.h
     * ^^^ this ends at 8
     * 9 is Mastodon: https://github.com/sailfishos/sailfish-account-mastodon/blob/e110ae0c4bbf9a0cee79d6faa7618d97f8da1339/eventsview-plugins/eventsview-plugin-mastodon/mastodon-delegate.qml#L23
     * lets claim 10
    */
    // socialNetwork: SocialSync.GitHub
    socialNetwork: 10
    dataType: SocialSync.Notifications
    providerName: "github"

    model: GitHubNotificationsModel {}

    delegate: GitHubFeedItem {
        accountId: model.accounts[0]
        userRemovable: true
        animateRemoval: defaultAnimateRemoval || delegateItem.removeAllInProgress

        onRemoveRequested: {
            delegateItem.model.remove(model.threadId)
        }

        onTriggered: {
            console.log("GHN triggered! Opening %1".arg(model.link))
            Qt.openUrlExternally(model.link)
        }

        Component.onCompleted: {
            refreshTimeCount = Qt.binding(function() { return delegateItem.refreshTimeCount })
            connectedToNetwork = Qt.binding(function() { return delegateItem.connectedToNetwork })
            eventsColumnMaxWidth = Qt.binding(function() { return delegateItem.eventsColumnMaxWidth })
        }
    }

    //% "Show more on GitHub"
    expandedLabel: qsTrId("lipstick-jolla-home-la-show-more-on-github")
    userRemovable: true

    onHeaderClicked: Qt.openUrlExternally("https://github.com/notifications")
    onExpandedClicked: Qt.openUrlExternally("https://github.com/notifications")

    onViewVisibleChanged: {
        if (viewVisible) {
            delegateItem.resetHasSyncableAccounts()
            delegateItem.model.refresh()
        }
    }
}
