import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Accounts 1.0
import com.jolla.settings.accounts 1.0

AccountCreationAgent {
    id: root

    property Item _oauthPage
    property Item _settingsDialog
    property QtObject _accountSetup
    property string _existingUserName

    function _handleAccountCreated(accountId, responseData) {
        var props = {
            "accessToken": responseData["AccessToken"],
            "accountId": accountId
        }
        var _accountSetup = accountSetupComponent.createObject(root, props)
        _accountSetup.done.connect(function() {
            accountCreated(accountId)
            _goToSettings(accountId)
        })
        _accountSetup.error.connect(function() {
            //: Error which is displayed when the user attempts to create a duplicate GitHub account
            //% "You have already added a GitHub account for user %1."
            //var duplicateAccountError = qsTrId("jolla_settings_accounts_extensions-la-github_duplicate_account").arg(root._existingUserName)
            var duplicateAccountError = "You have already added a GitHub account for user %1."
            accountCreationError(duplicateAccountError)
            _oauthPage.done(false, AccountFactory.BadParametersError, duplicateAccountError)
        })
    }

    function _goToSettings(accountId) {
        if (_settingsDialog != null) {
            _settingsDialog.destroy()
        }
        _settingsDialog = settingsComponent.createObject(root, {"accountId": accountId})
        pageStack.animatorReplace(_settingsDialog)
    }

    initialPage: AccountCreationLegaleseDialog {
        //: The text explaining how user's data will be backed up to GitHub
        //% "Adding a GitHub account on your device means that you agree to GitHub's Terms of Service."
        //legaleseText: qsTrId("jolla_settings_accounts_extensions-la-github-consent_text")
        legaleseText: "Adding a GitHub account on your device means that you agree to GitHub's Terms of Service."

        //: Button which the user presses to view GitHub Terms Of Service webpage
        //% "GitHub Terms of Service"
        //externalUrlText: qsTrId("jolla_settings_accounts_extensions-bt-github_terms")
        externalUrlText: "GitHub Terms of Service"
        externalUrlLink: "https://docs.github.com/en/site-policy/github-terms/github-terms-of-service"

        onStatusChanged: {
            if (status == PageStatus.Active) {
                if (_oauthPage != null) {
                    _oauthPage.destroy()
                }
                _oauthPage = oAuthComponent.createObject(root)
                acceptDestination = _oauthPage
            }
        }
    }

    AccountFactory {
        id: accountFactory
    }

    Component {
        id: accountSetupComponent
        QtObject {
            id: accountSetup
            property string accessToken
            property bool hasSetName
            property int accountId

            signal done()
            signal error()

            property Account newAccount: Account {
                id: account
                identifier: accountSetup.accountId
                onStatusChanged: {
                    if (status === Account.Initialized || status === Account.Synced) {
                        if (!accountSetup.hasSetName) {
                            getProfileInfo()
                        } else {
                            accountSetup.done()
                        }
                    } else if (status === Account.Invalid && accountSetup.hasSetName) {
                        accountSetup.error()
                    }
                }
            }

            function getProfileInfo() {
                var doc = new XMLHttpRequest()
                doc.onreadystatechange = function() {
                    if (doc.readyState === XMLHttpRequest.DONE) {
                        if (doc.status === 200) {
                            var user = JSON.parse(doc.responseText)
                            var name = user.name
                            var userId = user.login
                            var avatar = user.avatar_url

                            if (userId == null) {
                                // something went wrong, can't identify user
                                accountSetup.error()
                                return
                            }

                            if (name == null) {
                                name = ""
                            }

                            if (accountFactory.findAccount(
                                    "github",
                                    "",
                                    "default_credentials_id",
                                    userId) !== 0) {
                                // this account already exists. show error dialog.
                                hasSetName = true
                                root._existingUserName = name
                                newAccount.remove()
                                accountSetup.error()
                                return
                            }

                            newAccount.setConfigurationValue("", "default_credentials_username", name)
                            newAccount.setConfigurationValue("", "default_credentials_id", userId)
                            newAccount.displayName = name
                            newAccount.iconPath = avatar
                            accountSetup.hasSetName = true
                            newAccount.sync()
                        } else {
                            console.log("Failed to query GitHub user, error: " + doc.status)
                            accountSetup.done()
                        }
                    }
                }

                // deprecated, see https://developer.github.com/changes/2020-02-10-deprecating-auth-through-query-param/
                //var url = "https://api.github.com/user?access_token=" + accessToken
                var url = "https://api.github.com/user"
                doc.open("GET", url)
                doc.setRequestHeader('Accept', 'application/vnd.github+json');
                doc.setRequestHeader('X-GitHub-Api-Version', '2022-11-28');
                doc.setRequestHeader('Authorization', 'Bearer:' + accessToken);
                doc.send()
            }
        }
    }

    Component {
        id: oAuthComponent
        OAuthAccountSetupPage {
            Component.onCompleted: {
                var sessionData = {
                    "ClientId": keyProvider.storedKey("github", "", "client_id"),
                    "ClientSecret": keyProvider.storedKey("github", "", "client_secret")
                }
                console.debug("trying to set up:", JSON.stringify(sessionData))
                prepareAccountCreation(root.accountProvider, "github-posts", sessionData)
            }
            onAccountCreated: {
                root._handleAccountCreated(accountId, responseData)
                console.debug("success:", accountId, JSON.stringify(responseData,null,2))
            }
            onAccountCreationError: {
                root.accountCreationError(errorMessage)
            }

            StoredKeyProvider {
                id: keyProvider
            }
        }
    }

    Component {
        id: settingsComponent
        Dialog {
            property alias accountId: settingsDisplay.accountId

            acceptDestination: root.endDestination
            acceptDestinationAction: root.endDestinationAction
            acceptDestinationProperties: root.endDestinationProperties
            acceptDestinationReplaceTarget: root.endDestinationReplaceTarget
            backNavigation: false

            onAccepted: {
                root.delayDeletion = true
                settingsDisplay.saveAccountAndSync()
            }

            SilicaFlickable {
                anchors.fill: parent
                contentHeight: header.height + settingsDisplay.height

                DialogHeader {
                    id: header
                }

                GitHubSettingsDisplay {
                    id: settingsDisplay
                    anchors.top: header.bottom
                    accountManager: root.accountManager
                    accountProvider: root.accountProvider
                    autoEnableAccount: true

                    onAccountSaveCompleted: {
                        root.delayDeletion = false
                    }
                }

                VerticalScrollDecorator {}
            }
        }
    }
}
