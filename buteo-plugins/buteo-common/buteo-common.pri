# SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
# SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
# SPDX-FileCopyrightText: 2025,2026 Peter G. <sailfish@nephros.org>
#
# SPDX-License-Identifier: BSD-3-Clause

INCLUDEPATH += $$PWD
DEPENDPATH += .

QT += dbus

CONFIG += link_pkgconfig
PKGCONFIG += accounts-qt5 buteosyncfw5 socialcache libsignon-qt5 libsailfishkeyprovider

LIBS += -L$$PWD -lgithubbuteocommon

DEFINES += 'SYNC_DATABASE_DIR=\'\"Sync\"\''

