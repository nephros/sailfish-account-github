/*
 * Copyright (c) 2013 - 2020 Jolla Ltd.
 * Copyright (c) 2020 Open Mobile Platform LLC.
 *
 * License: Proprietary
 */

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
            var postSyncOptions = allSyncOptionsForService("github-posts")
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
                id: serviceSwitch
                checked: model.enabled
                text: model.displayName
                icon.source: model.iconName
                description: model.description
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
