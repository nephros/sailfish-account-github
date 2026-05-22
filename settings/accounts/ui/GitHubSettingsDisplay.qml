// SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
// SPDX-FileCopyrightText: 2025,2026 Peter G. <sailfish@nephros.org>
//
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Accounts 1.0
import com.jolla.settings.accounts 1.0

StandardAccountSettingsDisplay {
    id: root

    settingsModified: true // TODO only set to true when these settings have been modified

    onAboutToSaveAccount: {
        settingsLoader.updateAllSyncProfiles()
    }

    StandardAccountSettingsLoader {
        id: settingsLoader

        account: root.account
        accountProvider: root.accountProvider
        accountManager: root.accountManager
        autoEnableServices: root.autoEnableAccount

        onSettingsLoaded: {
            syncServicesRepeater.model = syncServices
            //otherServicesDisplay.serviceModel = otherServices

            // load the initial settings, using the first set of sync options as reference
            var postSyncOptions = allSyncOptionsForService("github-notifications")
            for (var profileId in postSyncOptions) {
                notificationSchedule.syncOptions = postSyncOptions[profileId]
                break
            }
        }
    }

    Column {
        id: syncServicesDisplay

        width: parent.width

        SectionHeader {
            text: qsTrId("settings-accounts-la-sync")
        }

        Repeater {
            id: syncServicesRepeater

            IconTextSwitch {
                checked: model.enabled
                icon.source: model.iconName
                text: model.serviceName === "github-notifications"
                           ? //% "Notifications"
                             qsTrId("settings-accounts-github-la-service_notifications")
                           : model.displayName
                description: model.serviceName === "github-notifications"
                           ? //% "Show GitHub notifications."
                             qsTrId("settings-accounts-github-la-service_notifications_description")
                           : ""
                visible: text.length > 0
                onCheckedChanged: {
                    if (checked) {
                        root.account.enableWithService(model.serviceName)
                    } else {
                        root.account.disableWithService(model.serviceName)
                    }
                }
            }
        }
    }

    /*
    AccountServiceSettingsDisplay {
        id: otherServicesDisplay

        enabled: root.accountEnabled
        onUpdateServiceEnabledStatus: {
            if (enabled) {
                root.account.enableWithService(serviceName)
            } else {
                root.account.disableWithService(serviceName)
            }
        }
    }
    */

    SyncScheduleOptions {
        id: notificationSchedule
        property QtObject syncOptions
        schedule: syncOptions ? syncOptions.schedule : null
    }

    Loader {
        width: parent.width
        active: notificationSchedule.syncOptions.schedule.enabled && notificationSchedule.syncOptions.schedule.peakScheduleEnabled

        sourceComponent: Component {
            PeakSyncOptions {
                schedule: notificationSchedule.syncOptions ? notificationSchedule.syncOptions.schedule : null
            }
        }
    }

}
