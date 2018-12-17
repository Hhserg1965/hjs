#-------------------------------------------------
#
# Project created by QtCreator 2012-11-30T08:35:26
#
#-------------------------------------------------

QT       -= gui
QT       -= core

#TARGET = levw
TEMPLATE = lib
TARGET = ../../bin/levw

SOURCES += levw.cpp

HEADERS += levw.h

unix:LIBS += -L../../lib/ -lv8 -lleveldb
INCLUDEPATH += ../../inc

#unix:INCLUDEPATH += /usr/local/BerkeleyDB.6.0/include

#unix:LIBS +=   -lleveldb

#win32:INCLUDEPATH += C:/opencv/build/include
#LIBS += -lmysqlclient
