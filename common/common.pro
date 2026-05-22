# SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
# SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

TEMPLATE = lib

QT -= gui
QT += sql

CONFIG += link_pkgconfig
PKGCONFIG += socialcache

TARGET = githubcommon
TARGET = $$qtLibraryTarget($$TARGET)

HEADERS += \
    $$PWD/githubnotificationsdatabase.h \
    $$PWD/githubnotificationsmodel.h

SOURCES += \
    $$PWD/githubnotificationsdatabase.cpp \
    $$PWD/githubnotificationsmodel.cpp

TARGETPATH = $$[QT_INSTALL_LIBS]
target.path = $$TARGETPATH

INSTALLS += target
