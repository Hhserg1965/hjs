#ifndef MYJS_H
#define MYJS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
//#include <QtCore>

#include <v8.h>
using namespace v8;

#ifdef _WIN32
#define MY_EXPORT __declspec(dllexport)
#else
#define MY_EXPORT
#endif

extern "C" {
  MY_EXPORT v8::Handle<v8::Value>  init(v8::Persistent<v8::Context> ctx,v8::Handle<v8::Value>(*out_buf)(char *b,int sz));
  MY_EXPORT v8::Handle<v8::Value> init_hh(v8::Persistent<v8::Context> ctx);
}

#ifdef _WIN32
//#include <windows.h>
#include <stdio.h>
#include <conio.h>

typedef void *HANDLE;

#else

//#include <QtGui>

#include <sys/ipc.h>
#include <sys/shm.h>

#define HANDLE int
#define LPTSTR char *

#endif

typedef struct {
    HANDLE	hMapFile;
    char *	pBuf;
    int		sz;

}HH_SHMEM;

class hBuf
{
public:
    char *p;

    unsigned long  sz;
    unsigned long  beg;
    unsigned long  end;

    int isSlice;

    HH_SHMEM    sh;

    hBuf(){p = 0;sz=beg=end=isSlice=0;sh.hMapFile=0; sh.pBuf =0; sh.sz=0;};
};

#endif
