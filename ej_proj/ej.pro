TEMPLATE = app
CONFIG += console
CONFIG -= qt

TARGET = ../bin/ej

QMAKE_CXXFLAGS += -std=c++0x

SOURCES += main.cpp \
    ../hh_shmem.c \
    ../rpi_sock.cpp \
    js_main.cpp \
    sha1.c \
    b64.cpp \
    ../hhprocess.cpp \
    mcachjs.cpp

#unix: LIBS += -lcurl -lz -luv -lv8

#LIBS += -lmc18
#LIBS += -L../lib -lmemcached -lmemcachedutil
#LIBS += -lmemcached

unix: LIBS += -L../lib  -lmemcached -lmemcachedutil -lz -luv -lv8 -lpq -lpthread
INCLUDEPATH += ../inc

HEADERS += \
    ../hh_shmem.h \
    ../rpi_sock.h \
    js_main.h \
    sha1.h \
    ../hhprocess.h \
    mcachjs.h
