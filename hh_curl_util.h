#ifndef HH_CURL_UTIL_H_INCLUDED
#define HH_CURL_UTIL_H_INCLUDED

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include <unistd.h>
#include <stddef.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>


//#include <stdint.h>
//#include <netinet/in.h>

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

#include <curl/curl.h>
#include <zlib.h>

#include <QtGui>

char * get_curl_url(char *ser_url,int *sz,char **hd,int *hsz);

typedef struct MemoryStruct {
  char *memory;
  size_t size;
} MemoryStructT;

class UrlRecive
{

public:

    std::string url;
    std::string eurl;
    std::string ctype;

    MemoryStructT h;
    MemoryStructT b;

    int result,cnt;
    long response_code;

    UrlRecive( std::string u,int c){
        url = u;
        result = -1;
        cnt = c;
        response_code = -1;

        h.memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */
        h.memory[0] = 0;
        h.size = 0;    /* no data at this point */

        b.memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */
        b.memory[0] = 0;
        b.size = 0;    /* no data at this point */
    };

    ~UrlRecive(){
//        std::cout << url << "-DELETED-\n";

        if( h.memory) free(h.memory);
        if( b.memory) free(b.memory);

    };

};typedef UrlRecive * UrlReciveP;

std::vector<UrlReciveP> *hh_curl_mul(std::vector<std::string> a, int mth);
UrlReciveP get_curl_url_new(char *ser_url);

#endif // HH_CURL_UTIL_H_INCLUDED
