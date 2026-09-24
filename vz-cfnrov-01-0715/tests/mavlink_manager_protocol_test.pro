QT += core
QT -= gui

CONFIG += console c++11
CONFIG -= app_bundle

TEMPLATE = app
TARGET = mavlink_manager_protocol_test

DEFINES += MAVLINK_ALIGNED_FIELDS=0 _USE_MATH_DEFINES

INCLUDEPATH += $$PWD/.. \
               $$PWD/../mavlink \
               $$PWD/../thirdparty/mavlink/v2.0/ardupilotmega

SOURCES += $$PWD/mavlink_manager_protocol_test.cpp \
           $$PWD/../mavlink/mavlink_manager.cpp

HEADERS += $$PWD/../mavlink/mavlink_manager.h
