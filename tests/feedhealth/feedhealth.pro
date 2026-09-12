QT = core testlib sql
CONFIG += console testcase c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = feedhealth-test
INCLUDEPATH += ../../src/feedsview
SOURCES += test_feedhealth.cpp ../../src/feedsview/feedhealth.cpp
HEADERS += ../../src/feedsview/feedhealth.h
