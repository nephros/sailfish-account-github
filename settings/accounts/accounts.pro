# SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
# SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
# SPDX-FileCopyrightText: 2025,2026 Peter G. <sailfish@nephros.org>
#
# SPDX-License-Identifier: BSD-3-Clause

TEMPLATE = aux

TS_FILE = $$OUT_PWD/settings-accounts-github.ts
EE_QM = $$OUT_PWD/settings-accounts-github_eng_en.qm

ts.commands += lupdate $$PWD/ui -ts $$TS_FILE
ts.CONFIG += no_check_exist no_link
ts.output = $$TS_FILE
ts.input = .

ts_install.files = $$TS_FILE
ts_install.path = /usr/share/translations/source
ts_install.CONFIG += no_check_exist

engineering_english.commands += lrelease -idbased $$TS_FILE -qm $$EE_QM
engineering_english.CONFIG += no_check_exist no_link
engineering_english.depends = ts
engineering_english.input = $$TS_FILE
engineering_english.output = $$EE_QM

engineering_english_install.path = /usr/share/translations
engineering_english_install.files = $$EE_QM
engineering_english_install.CONFIG += no_check_exist

QMAKE_EXTRA_TARGETS += ts engineering_english
PRE_TARGETDEPS += ts engineering_english

OTHER_FILES += \
    $$PWD/providers/github.provider \
    $$PWD/services/github-notifications.service \
    $$PWD/ui/GitHubSettingsDisplay.qml \
    $$PWD/ui/github.qml \
    $$PWD/ui/github-settings.qml \
    $$PWD/ui/github-update.qml

provider.files += $$PWD/providers/github.provider
provider.path = /usr/share/accounts/providers/

services.files += \
    $$PWD/services/github-notifications.service \
services.path = /usr/share/accounts/services/

ui.files += \
    $$PWD/ui/GitHubSettingsDisplay.qml \
    $$PWD/ui/github.qml \
    $$PWD/ui/github-settings.qml \
    $$PWD/ui/github-update.qml
ui.path = /usr/share/accounts/ui/

INSTALLS += provider services ui ts_install engineering_english_install
