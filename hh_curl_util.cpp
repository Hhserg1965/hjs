#include "hh_curl_util.h"

#include "ua.h"

//extern CURL *curl_handle;

/*
typedef struct MemoryStruct {
  char *memory;
  size_t size;
} MemoryStructT;
*/

static size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)data;

  mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory == NULL) {
	/* out of memory! */
	printf("not enough memory (realloc returned NULL)\n");
	exit(EXIT_FAILURE);
  }

  memcpy(&(mem->memory[mem->size]), ptr, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

int curl_set_timeout = 20;
int curl_set_connection_timeout = 10;
int curl_set_dns_timeout = 10;
int curl_set_low_speed_limit = 0;
int curl_set_low_speed_time = 0;
int curl_set_max_red = 6;
char cookiestring[5000] = "";

char curl_set_proxystring[5000] = "";
char curl_set_proxytype[5000] = "";

CURL *curl_hd = 0;
static int curl_cnt = 0;
#define curl_max_urls  300

void hh_curl_set_opt(CURL *curl_handle)
{
    int cur_ua = 0;

    //---

    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L) ;
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L) ;
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYSTATUS, 0L) ;
    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0L);


    /* enable TCP keep-alive for this transfer */
    curl_easy_setopt(curl_handle, CURLOPT_TCP_KEEPALIVE, 1L);

    /* keep-alive idle time to 120 seconds */
    curl_easy_setopt(curl_handle, CURLOPT_TCP_KEEPIDLE, 120L);

    /* interval time between keep-alive probes: 60 seconds */
    curl_easy_setopt(curl_handle, CURLOPT_TCP_KEEPINTVL, 60L);

    /* send all data to this function  */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, WriteMemoryCallback);

    if (strlen(cookiestring) > 0) {
        curl_easy_setopt(curl_handle, CURLOPT_COOKIE, cookiestring);
//        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, cookiestring );
//        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, strlen(cookiestring));
    }

    if (strlen(curl_set_proxystring) > 0) {

        curl_easy_setopt(curl_handle,CURLOPT_PROXY,curl_set_proxystring);
        if (strcmp(curl_set_proxytype,"socks5h") == 0) {
            curl_easy_setopt(curl_handle,CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5_HOSTNAME);
        } else if (strcmp(curl_set_proxytype,"socks5") == 0) {
            curl_easy_setopt(curl_handle,CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5);
        } else if (strcmp(curl_set_proxytype,"socks4") == 0) {
            curl_easy_setopt(curl_handle,CURLOPT_PROXYTYPE, CURLPROXY_SOCKS4);
        }
    }

    curl_easy_setopt(curl_handle, CURLOPT_AUTOREFERER, 1);
    curl_easy_setopt(curl_handle, CURLOPT_ACCEPT_ENCODING, "");

    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
    //curl_easy_setopt(curl_handle, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS) ;
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L) ;
    curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS, curl_set_max_red) ;

    /* some servers don't like requests that are made without a user-agent
       field, so we provide one */

    cur_ua = ((curl_cnt++)/curl_max_urls) % uaSize;
//    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.11 (KHTML, like Gecko) Chrome/23.0.1271.91 Safari/537.11");
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, ua[cur_ua]);


    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, curl_set_timeout);

    if( curl_set_connection_timeout) curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, curl_set_connection_timeout);
    if( curl_set_dns_timeout) curl_easy_setopt(curl_handle, CURLOPT_DNS_CACHE_TIMEOUT, curl_set_dns_timeout);

    if( curl_set_low_speed_limit) curl_easy_setopt(curl_handle, CURLOPT_LOW_SPEED_LIMIT, curl_set_low_speed_limit);
    if( curl_set_low_speed_time) curl_easy_setopt(curl_handle, CURLOPT_LOW_SPEED_TIME, curl_set_low_speed_time);

}

char * get_curl_url(char *ser_url,int *sz,char **hd,int *hsz)
{
    CURL *curl_handle;
    CURLcode res;
    int cur_ua = 0;

	struct MemoryStruct chunk;

	chunk.memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */
	chunk.memory[0] = 0;
	chunk.size = 0;    /* no data at this point */

	struct MemoryStruct chunk_h;

	chunk_h.memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */
	chunk_h.memory[0] = 0;
	chunk_h.size = 0;    /* no data at this point */

	/* init the curl session */
//    curl_global_init(CURL_GLOBAL_ALL);

    if( !curl_hd) {
        curl_hd = curl_handle = curl_easy_init();
    }else{
        curl_handle = curl_hd;
    }

	curl_easy_setopt( curl_handle, CURLOPT_URL, ser_url);

    hh_curl_set_opt( curl_handle);

	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEHEADER, (void *)&chunk_h);

    /* get it! */
    res = curl_easy_perform(curl_handle);

    /* check for errors */
     if(res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %d %s\n", res, curl_easy_strerror(res));

        chunk.memory[0] = 0;
        chunk_h.memory[0] = 0;
        chunk.size = 0;
        chunk_h.size = 0;
     }

	/* cleanup curl stuff */

//    curl_easy_cleanup(curl_handle);

//    curl_global_cleanup();

/*
	char *utf_p = strstr(chunk.memory,"charset=");
	if(utf_p) {
		if( !strnicmp(utf_p+8,"utf-8",5) ) {
			char *new_body = rd(QString::fromUtf8(chunk.memory));
			free(chunk.memory);
			chunk.size = strlen(new_body);
			chunk.memory = (char *)malloc(chunk.size + 1);
			strcpy(chunk.memory,new_body);
		}
	}
*/
    if( sz) {
		*sz = chunk.size;
	}

    if( hsz) {
        *hsz = chunk_h.size;
    }

    if( hd) {
        *hd = chunk_h.memory;
    }else {
        free(chunk_h.memory);
    }
/*
	if(chunk_h.size >0) {
	    printf("HEADER:\n{%s}\n",chunk_h.memory);
	}
*/

	return (chunk.memory);
}

//--------------------

UrlReciveP get_curl_url_new(char *ser_url)
{
    CURL *curl_handle;
    CURLcode res;
    int cur_ua = 0;

    UrlRecive *r = new UrlRecive( ser_url,0);

    if( !curl_hd) {
        curl_hd = curl_handle = curl_easy_init();
    }else{
        curl_handle = curl_hd;
    }

    curl_easy_setopt( curl_handle, CURLOPT_URL, ser_url);
    hh_curl_set_opt( curl_handle);

    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&r->b);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEHEADER, (void *)&r->h);

    /* get it! */
    res = curl_easy_perform(curl_handle);

    long response_code;
    curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
    r->response_code = response_code;
    r->result = res;

    char *urlp = NULL;
    curl_easy_getinfo(curl_handle, CURLINFO_EFFECTIVE_URL, &urlp);
    if( urlp) r->eurl = urlp;

    char *ctype = NULL;
    curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &ctype);
    if( ctype) r->ctype = ctype;


    return ( r);
}

//--------------------

static void hhcurle_init(CURLM *cm, UrlRecive* d)
{
  int cur_ua = 0;

  CURL *eh = curl_easy_init();

  curl_easy_setopt(eh, CURLOPT_URL, d->url.c_str());

  hh_curl_set_opt( eh);

  curl_easy_setopt(eh, CURLOPT_WRITEDATA, (void *)&d->b);
  curl_easy_setopt(eh, CURLOPT_WRITEHEADER, (void *)&d->h);
  curl_easy_setopt(eh, CURLOPT_PRIVATE, d);

/*


  curl_easy_setopt(eh, CURLOPT_TCP_KEEPALIVE, 1L);
  curl_easy_setopt(eh, CURLOPT_TCP_KEEPIDLE, 120L);
  curl_easy_setopt(eh, CURLOPT_TCP_KEEPINTVL, 60L);
  curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  curl_easy_setopt(eh, CURLOPT_WRITEDATA, (void *)&d->b);
  curl_easy_setopt(eh, CURLOPT_HEADERFUNCTION, WriteMemoryCallback);
  curl_easy_setopt(eh, CURLOPT_WRITEHEADER, (void *)&d->h);
  curl_easy_setopt(eh, CURLOPT_FOLLOWLOCATION, 1L) ;
  curl_easy_setopt(eh, CURLOPT_MAXREDIRS, curl_set_max_red) ;

  curl_easy_setopt(eh, CURLOPT_SSL_VERIFYHOST, 0L) ;
  curl_easy_setopt(eh, CURLOPT_SSL_VERIFYPEER, 0L) ;
  curl_easy_setopt(eh, CURLOPT_SSL_VERIFYSTATUS, 0L) ;

  curl_easy_setopt(eh, CURLOPT_AUTOREFERER, 1);
  curl_easy_setopt(eh, CURLOPT_NOPROGRESS, 1L);


//  curl_easy_setopt(eh, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.11 (KHTML, like Gecko) Chrome/23.0.1271.91 Safari/537.11");
  cur_ua = ((curl_cnt++)/curl_max_urls) % uaSize;
//    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.11 (KHTML, like Gecko) Chrome/23.0.1271.91 Safari/537.11");
  curl_easy_setopt(eh, CURLOPT_USERAGENT, ua[cur_ua]);

  //--

  curl_easy_setopt(eh, CURLOPT_HEADER, 0L);
  curl_easy_setopt(eh, CURLOPT_URL, d->url.c_str());
//  curl_easy_setopt(eh, CURLOPT_PRIVATE, d->cnt);
  curl_easy_setopt(eh, CURLOPT_PRIVATE, d);
  curl_easy_setopt(eh, CURLOPT_VERBOSE, 0L);

  //--

  if (strlen(cookiestring) > 0) {
      curl_easy_setopt(eh, CURLOPT_COOKIE, cookiestring);
//        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, cookiestring );
//        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, strlen(cookiestring));
  }

  if (strlen(curl_set_proxystring) > 0) {

      curl_easy_setopt(eh,CURLOPT_PROXY,curl_set_proxystring);
      if (strcmp(curl_set_proxytype,"socks5h") == 0) {
          curl_easy_setopt(eh,CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5_HOSTNAME);
      } else if (strcmp(curl_set_proxytype,"socks5") == 0) {
          curl_easy_setopt(eh,CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5);
      } else if (strcmp(curl_set_proxytype,"socks4") == 0) {
          curl_easy_setopt(eh,CURLOPT_PROXYTYPE, CURLPROXY_SOCKS4);
      }
  }

  if( curl_set_connection_timeout) curl_easy_setopt(eh, CURLOPT_CONNECTTIMEOUT, curl_set_connection_timeout);
  if( curl_set_dns_timeout) curl_easy_setopt(eh, CURLOPT_DNS_CACHE_TIMEOUT, curl_set_dns_timeout);

  if( curl_set_low_speed_limit) curl_easy_setopt(eh, CURLOPT_LOW_SPEED_LIMIT, curl_set_low_speed_limit);
  if( curl_set_low_speed_time) curl_easy_setopt(eh, CURLOPT_LOW_SPEED_TIME, curl_set_low_speed_time);
*/
  curl_multi_add_handle(cm, eh);
}

std::vector<UrlReciveP> *hh_curl_mul(std::vector<std::string> a, int mth){

    std::vector<UrlReciveP> *r = new std::vector<UrlReciveP>();

    for( int n =0; n < a.size(); ++n ) {
//        std::cout << a[n] << '\n';

        r->push_back( new UrlRecive(a[n],n));
    }

    int ccc = 0;

    CURLM *cm;
    CURLMsg *msg;
    long L;
    unsigned int C=0;
    int M, Q, U = -1;
    fd_set R, W, E;
    struct timeval T;

    cm = curl_multi_init();

    /* we can optionally limit the total amount of connections this multi handle
       uses */
    curl_multi_setopt(cm, CURLMOPT_MAXCONNECTS, (long)mth);

    for(C = 0; (C < mth) && (C < r->size()); ++C) {
      hhcurle_init(cm, r->at(C));
    }

    while(U) {
      curl_multi_perform(cm, &U);

      if(U) {
        FD_ZERO(&R);
        FD_ZERO(&W);
        FD_ZERO(&E);

        if(curl_multi_fdset(cm, &R, &W, &E, &M)) {
          fprintf(stderr, "E: curl_multi_fdset\n");
          return r;
        }

        if(curl_multi_timeout(cm, &L)) {
          fprintf(stderr, "E: curl_multi_timeout\n");
          return r;
        }
        if(L == -1)
          L = 100;

        if(M == -1)  {
  #ifdef WIN32
          Sleep(L);
  #else
          sleep((unsigned int)L / 1000);
  #endif
        }
        else {
          T.tv_sec = L/1000;
          T.tv_usec = (L%1000)*1000;

          if(0 > select(M+1, &R, &W, &E, &T)) {
            fprintf(stderr, "E: select(%i,,,,%li): %i: %s\n",
                M+1, L, errno, strerror(errno));
            return r;
          }
        }
      }

      while((msg = curl_multi_info_read(cm, &Q))) {
        if(msg->msg == CURLMSG_DONE) {
//          int d;
          UrlRecive *d;
          CURL *e = msg->easy_handle;
          curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &d);

          long response_code;
          curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &response_code);
          d->response_code = response_code;

//          r->at((int)d)->result = msg->data.result;
          d->result = msg->data.result;

          char *urlp = NULL;
          curl_easy_getinfo(msg->easy_handle, CURLINFO_EFFECTIVE_URL, &urlp);
          if( urlp) d->eurl = urlp;

          char *ctype = NULL;
          curl_easy_getinfo(msg->easy_handle, CURLINFO_CONTENT_TYPE, &ctype);
          if( ctype) d->ctype = ctype;


//          fprintf(stderr, "RRR: %d %d %d %d - %s <%s>\n",d->cnt,r->size(),ccc++,
//                  msg->data.result, curl_easy_strerror(msg->data.result),  d->url.c_str()); // d->url.c_str());
          curl_multi_remove_handle(cm, e);
          curl_easy_cleanup(e);
        }
        else {
          fprintf(stderr, "E: CURLMsg (%d)\n", msg->msg);
        }
        if(C < r->size()) {
          hhcurle_init(cm, r->at(C++));
          U++; /* just to prevent it from remaining at 0 if there are more
                  URLs to get */
        }
      }
    }

    curl_multi_cleanup(cm);

    return r;
}
