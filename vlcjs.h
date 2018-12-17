#ifndef VLCJS_H
#define VLCJS_H

#include <QtGui>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include<time.h>

#include <v8.h>
using namespace v8;

#include <./vlc/vlc.h>
#include <./vlc/libvlc.h>

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

#ifdef _WIN32
#define MY_EXPORT __declspec(dllexport)
#else
#define MY_EXPORT
#endif

extern "C" {

    MY_EXPORT v8::Handle<v8::Value>  init(v8::Persistent<v8::Context> ctx);
//    MY_EXPORT int  init_hh(int ctx){ return(0);}
}

#endif // VLCJS_H
