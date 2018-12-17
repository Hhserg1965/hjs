#ifndef BB_H
#define BB_H
// Bitches Brew

#include "mainwindow.h"
#include "hh_shmem.h"
#include "hh_curl_util.h"
#include "hhprocess.h"
#include "bloom.h"
#include "rpi_sock.h"
#include "ctype.h"

#include <stdio.h>
#include <string.h>

//#include <publib.h>
//#include <sdgstd.h>

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

class bb
{
public:
    bb();
};

#endif // BB_H
