#ifndef JS_MAIN_H
#define JS_MAIN_H

#include <iostream>
#include "uv.h"
#include <stdio.h>
#include <string.h>
#include<zlib.h>
#include<time.h>
#include<stdlib.h>
#include <signal.h>

#ifndef _WIN32
#include <stdint.h>
#include <unistd.h>
#include <netinet/in.h>
#define stricmp strcasecmp
#define strcmpi strcasecmp
#define strnicmp strncasecmp
#include <netdb.h>
#include <sys/socket.h>
#endif

#include <v8.h>

#include "../hh_shmem.h"
#include "../rpi_sock.h"
#include "../hhprocess.h"
#include <libpq-fe.h>
#include "semaphore.h"
#include "fcntl.h"
#include "stdio.h"

//#include <pg_type.h>

//using namespace std;

using namespace v8;

extern v8::Persistent<v8::Context> context;
extern int quit_flg;

#ifdef _WIN32
#else
#include <signal.h>
#include <sys/wait.h>
#endif

#ifdef _WIN32
#include <direct.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

#define MSLEEP(x) Sleep(x)

#else

#define MSLEEP(x) usleep(x*1000)

#endif

extern "C" {
#include <libmemcached/memcached.h>
#include <libmemcached/util.h>
#include "sha1.h"
}
#include<zlib.h>

#include "../rpi_sock.h"
#include "../hh_shmem.h"
#include <stdint.h>

#include <iostream>
#include <map>    //подключили библиотеку для работы с map
//using namespace std;

typedef std::map<std::string, std::string> HHMapSS_T;
typedef std::map<std::string, int64_t> HHMapSN_T;
typedef std::map<int64_t, std::string> HHMapNS_T;

typedef HHMapSS_T::iterator HHMapSS_TI;
typedef HHMapSN_T::iterator HHMapSN_TI;
typedef HHMapNS_T::iterator HHMapNS_TI;

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

class SqlAdd
{
public:
    PGresult   *res;
    char*   sql;
    int flg_exec;
    int con;
    int pos;
    char stnm[100];

    SqlAdd(){
        res = NULL;
        sql = NULL;
        flg_exec = 0;
        con = 0;
        pos = 0;
        stnm[0] = 0;
    }
};


#define QBOOLOID 16
#define QINT8OID 20
#define QINT2OID 21
#define QINT4OID 23
#define QNUMERICOID 1700
#define QFLOAT4OID 700
#define QFLOAT8OID 701
#define QABSTIMEOID 702
#define QRELTIMEOID 703
#define QDATEOID 1082
#define QTIMEOID 1083
#define QTIMETZOID 1266
#define QTIMESTAMPOID 1114
#define QTIMESTAMPTZOID 1184
#define QOIDOID 2278
#define QBYTEAOID 17
#define QREGPROCOID 24
#define QXIDOID 28
#define QCIDOID 29

#endif // JS_MAIN_H
