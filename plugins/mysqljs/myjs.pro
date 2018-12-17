#-------------------------------------------------
#
# Project created by QtCreator 2012-11-30T08:35:26
#
#-------------------------------------------------

QT       -= gui
QT       += core

TARGET = ../../bin/myjs
TEMPLATE = lib

INCLUDEPATH += ../../inc

#DEFINES += CVW_LIBRARY

SOURCES += myjs.cpp

HEADERS += myjs.h

#unix:LIBS += -lv8

#unix:LIBS +=   -lopencv_core -lopencv_gpu -lopencv_imgproc -lopencv_objdetect -lopencv_video

#win32:INCLUDEPATH += C:/v8/include
#win32:LIBS += -LC:/v8/build/Release/lib/ -lv8
#win32:LIBS +=  -lWs2_32  -lwinmm

#win32:INCLUDEPATH += C:/opencv/build/include

LIBS += -L../../lib/ -lmysqlclient -lv8
