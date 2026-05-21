# SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
# SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
# SPDX-FileCopyrightText: 2025,2026 Peter G. <sailfish@nephros.org>
#
# SPDX-License-Identifier: BSD-3-Clause

TARGET = github-notifications-client

QT -= gui

include($$PWD/../buteo-common/buteo-common.pri)
include($$PWD/../../common/common.pri)

TS_FILE = $$OUT_PWD/lipstick-jolla-home-github-notifications.ts
EE_QM = $$OUT_PWD/lipstick-jolla-home-github-notifications_eng_en.qm

ts.commands += lupdate $$PWD -ts $$TS_FILE
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

CONFIG += link_pkgconfig
PKGCONFIG += mlite5 nemonotifications-qt5

INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/githubnotificationsplugin.cpp \
    $$PWD/githubnotificationssyncadaptor.cpp

HEADERS += \
    $$PWD/githubnotificationsplugin.h \
    $$PWD/githubnotificationssyncadaptor.h

OTHER_FILES += \
    $$PWD/github-notifications.xml \
    $$PWD/github.Notifications.xml

TEMPLATE = lib
CONFIG += plugin
target.path = $$[QT_INSTALL_LIBS]/buteo-plugins-qt5/oopp

sync.path = /etc/buteo/profiles/sync
sync.files = github.Notifications.xml

client.path = /etc/buteo/profiles/client
client.files = github-notifications.xml

INSTALLS += target sync client ts_install engineering_english_install
