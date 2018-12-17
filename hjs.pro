#-------------------------------------------------
#
# Project created by QtCreator 2012-11-06T10:25:11
#
#-------------------------------------------------

QT       += core gui sql webkit

QMAKE_CXXFLAGS += -std=c++0x

TARGET = ./bin/hjs
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
	gui.cpp \
	hio.cpp \
	bb.cpp \
	hh_shmem.c \
	hhprocess.cpp \
	hh_curl_util.cpp \
	bloom.c \
	rpi_sock.cpp \
        jsmn.c

HEADERS  += mainwindow.h \
	gui.h \
	hio.h \
	bb.h \
	hh_shmem.h \
	hhprocess.h \
	hh_curl_util.h \
	bloom.h \
	rpi_sock.h \
        jsmn.h

FORMS    += mainwindow.ui

RESOURCES += \
	hjs.qrc

INCLUDEPATH += ./inc
#LIBS += /home/hh/v8_rt/libv8_base.a
#LIBS += /home/hh/v8_rt/libv8_snapshot.a
#LIBS += /home/hh/v8_rt/libv8.so

unix:LIBS += -L./lib -lv8
#DEFINES += V8_SHARED
#DEFINES +=JSMN_PARENT_LINKS

unix: LIBS += -L./ -lcurl -lz

win32:INCLUDEPATH += I:/libs_zlib/include
win32:LIBS += -LI:/libs_zlib/ZlibStatRelease/ -lzlibstat

win32:INCLUDEPATH += C:/v8/include
#win32:LIBS += -LC:/v8/build/Release/lib/  -lv8_base -lWs2_32  -lwinmm -lv8_snapshot
win32:LIBS += -LC:/v8/build/Release/lib/ -lv8
win32:LIBS +=  -lWs2_32  -lwinmm

win32:INCLUDEPATH += I:/libs_cURL/include
win32:LIBS += -LI:/libs_cURL/DLL-Release/ -llibcurl_imp

OTHER_FILES +=


#LIBS +=   -lopencv_core -lopencv_gpu -lopencv_imgproc -lopencv_objdetect -lopencv_video

DISTFILES +=
