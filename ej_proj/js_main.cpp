#include "js_main.h"

Persistent<v8::ObjectTemplate> bb_template;
Persistent<v8::ObjectTemplate> buf_template;
Persistent<v8::ObjectTemplate> sock_template;
Persistent<v8::ObjectTemplate> proc_template;
Persistent<v8::ObjectTemplate> file_template;
Persistent<v8::ObjectTemplate> sql_template;
Persistent<v8::ObjectTemplate> lock_template;

Persistent<v8::ObjectTemplate> hash_template;
Persistent<v8::ObjectTemplate> map_template;

#define MAX_RD_DB_SIZE 10000000

char * get_file_text_utf8(char * fn,int *fsz);

extern int argc_;
extern char **argv_;
extern int run_process;

static v8::Handle<v8::Value>pclone(const Arguments &args);


//-------------- db

typedef struct {
    char nm[100];
    PGconn *con;
}PGDB_T;

#define PGDB_T_MAX 50

PGDB_T pgdb[PGDB_T_MAX];

enum {
Buf_ID=20000,Sql_ID,PSql_ID,Socket_ID,Proc_ID,File_ID,HashSS_ID,HashSN_ID,HashNS_ID,MapSS_ID,MapSN_ID,MapNS_ID,Lock_ID
};
static int cnt_buf_w = 0;

static void objectWeakCallback(v8::Persistent<v8::Value> obj, void* parameter)
{
    HandleScope handle_scope;

//fprintf(stderr,"objectWeakCallback -->\n");fflush(stderr);

    v8::Persistent<v8::Object> extObj = v8::Persistent<v8::Object>::Cast(obj);
//fprintf(stderr,"objectWeakCallback 1-->\n");fflush(stderr);
    void * par = (void *)(Handle<External>::Cast(extObj->GetInternalField(0)))->Value();
//fprintf(stderr,"objectWeakCallback 2-->\n");fflush(stderr);
    v8::Handle<v8::External> extObjID = v8::Handle<v8::External>::Cast(extObj->GetInternalField(1));
//fprintf(stderr,"objectWeakCallback 3-->\n");fflush(stderr);

    switch (extObjID->Int32Value())
    {
        case Buf_ID:
        {
            hBuf *o = (hBuf*) par;

//fprintf(stderr,"Buf_ID** -->%d %d\n",++cnt_buf_w,o->sz);fflush(stderr);

            if( !o->isSlice && o->p && !o->sh.pBuf) {
//fprintf(stderr,"Buf_ID -->%d %d %s\n",++cnt_buf_w,o->sz,o->p);fflush(stderr);
                free(o->p);
            }
            if( !o->isSlice && o->sh.pBuf) hh_shmem_close(&o->sh);

            delete o;

            break;
        }
        case PSql_ID:
        {
            SqlAdd   * p = (SqlAdd   * )((Handle<External>::Cast(extObj->GetInternalField(0)))->Value());

            if( p->res) {
                PQclear(p->res);
                p->res = 0;
            }
            if( p->sql) {
                free(p->sql);
                p->sql = 0;
            }
            if( p->stnm[0] != 0) {
                char b[300];
                sprintf(b,"DEALLOCATE %s",p->stnm);
                PGresult *res = PQexec( pgdb[p->con].con, b);
                if( PQresultStatus(res) == PGRES_FATAL_ERROR) {
                    fprintf(stderr, "PQexec <%s> failed: %s %d\n",b,PQerrorMessage(pgdb[p->con].con),PQresultStatus(res));fflush(stderr);
                }
                PQclear(res);
            }

            delete p;
        }
        break;
/*
        case Sql_ID:
        {
            QSqlQuery *o = (QSqlQuery*) par;
            if( o) {
                o->clear();
                delete o;
            }
//            fprintf(stderr,"Sql_ID delete %ld\n",o);fflush(stderr);

            break;
        }
*/
        case Socket_ID:
        {
            fprintf(stderr,"Socket_ID delete -->\n");fflush(stderr);

            int sc = (int) (Handle<External>::Cast(extObj->GetInternalField(0)))->Int32Value();
            fprintf(stderr,"Socket_ID delete %d\n",sc);fflush(stderr);
//            if( sc >= 0) {hh_closesocket(sc);}

            break;
        }
/*
        case Proc_ID:
        {
            hhProcess *o = (hhProcess*) par;
            if( o) {
                o->close();
                delete o;
            }

            break;
        }
*/
    }

    obj.ClearWeak();
//    obj.Dispose();
    obj.Clear();
}

static v8::Handle<v8::Value>v8_js_print_lg_err(const Arguments &args)
{
    HandleScope handle_scope;
    Local<String> r = String::NewSymbol("");

    for (int i = 0; i < args.Length(); ++i){
        String::Utf8Value  ascii(args[i]);
        Local<String> a = String::NewSymbol(*ascii);
        r = String::Concat(r,a);
        r = String::Concat(r,String::NewSymbol(" "));
    }
    String::Utf8Value  rr(r);

    fprintf(stderr,"ERR:: %s\n",*rr);fflush(stderr);
    return handle_scope.Close(Boolean::New(true));
}

static int is_log = 0;

static v8::Handle<v8::Value>v8_js_print_lg(const Arguments &args)
{
    HandleScope handle_scope;
//    QString result = "";
    Local<String> r = String::NewSymbol("");

    if( args.Length() == 0) {
        is_log = -1;
        return handle_scope.Close(Boolean::New(true));
    }
    if( args[0]->IsNull()) {
        is_log = 0;
        return handle_scope.Close(Boolean::New(true));
    }

    if( is_log) {
        return handle_scope.Close(Boolean::New(true));
    }

    for (int i = 0; i < args.Length(); ++i){
        String::Utf8Value  ascii(args[i]);
        Local<String> a = String::NewSymbol(*ascii);
        r = String::Concat(r,a);
        r = String::Concat(r,String::NewSymbol(" "));
    }
    String::Utf8Value  rr(r);

    fprintf(stderr,"%s\n",*rr);fflush(stderr);
    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>hjs_sleep(const Arguments &args)
{
    HandleScope handle_scope;

    int tm_max = 300;

    if( args.Length() > 0) {
        tm_max = args[0]->Int32Value();
        if(tm_max < 0) tm_max = 0;
    }

    MSLEEP(tm_max);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>mustQuit(const Arguments &args)
{
    HandleScope handle_scope;

    if( quit_flg) return handle_scope.Close(Boolean::New(true));

    return handle_scope.Close(Boolean::New(false));
}
static v8::Handle<v8::Value>hh_strerror(const Arguments &args)
{
    HandleScope handle_scope;

    return handle_scope.Close(String::New(strerror(errno)));
}
static v8::Handle<v8::Value>hjs_A2i(const Arguments &args)
{
    HandleScope handle_scope;

    long zn = 0l;

    if( args.Length() > 0) {
        String::Utf8Value   s(args[0]);
        zn = atol(*s);
    }

    return handle_scope.Close(Number::New(zn));
}
static v8::Handle<v8::Value>hjs_A2f(const Arguments &args)
{
    HandleScope handle_scope;

    double zn = 0;

    if( args.Length() > 0) {
        String::Utf8Value   s(args[0]);
        zn = atof(*s);
    }

    return handle_scope.Close(Number::New(zn));
}

static v8::Handle<v8::Value>idle(const Arguments &args)
{
    HandleScope handle_scope;

    while( !v8::V8::IdleNotification()){}
    v8::V8::LowMemoryNotification();

    return handle_scope.Close(Boolean::New(true));
}
static v8::Handle<v8::Value>inc(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    String::Utf8Value fn(args[0]);

    v8::TryCatch trycatch;

    int fsz=0;
    char *fb = get_file_text_utf8(*fn,&fsz);
    if( !fb) return handle_scope.Close(Boolean::New(false));

    Local<Value> result;
    Local<Script> script = Script::Compile(v8::String::New(fb)); // компилируем
    if (trycatch.HasCaught()) {
        String::Utf8Value err(trycatch.Message()->Get());
        fprintf(stderr,"COMPILE ERROR: %s\n",*err);fflush(stderr);
        String::Utf8Value exc(trycatch.Exception());
        fprintf(stderr,"COMPILE ERROR - EXEPTION: %s\n",*exc);fflush(stderr);
        String::Utf8Value st(trycatch.StackTrace());
        fprintf(stderr,"COMPILE ERROR - STACK: %s\n",*st);fflush(stderr);

        free(fb);
        return handle_scope.Close(Boolean::New(false));
    }
    result = script->Run(); // выполняем
    if (trycatch.HasCaught()) {
        String::Utf8Value err(trycatch.Message()->Get());
        fprintf(stderr,"RUN ERROR: %s\n",*err);fflush(stderr);
        String::Utf8Value exc(trycatch.Exception());
        fprintf(stderr,"RUN ERROR - EXEPTION: %s\n",*exc);fflush(stderr);
        String::Utf8Value st(trycatch.StackTrace());
        fprintf(stderr,"RUN ERROR - STACK: %s\n",*st);fflush(stderr);

        free(fb);
        return handle_scope.Close(Boolean::New(false));
    }

    free(fb);

    return handle_scope.Close(result);
}
static v8::Handle<v8::Value>parseJS(char *fb)
{
    HandleScope handle_scope;

    v8::TryCatch trycatch;

    Local<Value> result;
    Local<Script> script = Script::Compile(v8::String::New(fb)); // компилируем
    if (trycatch.HasCaught()) {
        String::Utf8Value err(trycatch.Message()->Get());
        fprintf(stderr,"COMPILE ERROR: %s\n",*err);fflush(stderr);
        String::Utf8Value exc(trycatch.Exception());
        fprintf(stderr,"COMPILE ERROR - EXEPTION: %s\n",*exc);fflush(stderr);
        String::Utf8Value st(trycatch.StackTrace());
        fprintf(stderr,"COMPILE ERROR - STACK: %s\n",*st);fflush(stderr);

        return handle_scope.Close(Boolean::New(false));
    }
    result = script->Run(); // выполняем
    if (trycatch.HasCaught()) {
        String::Utf8Value err(trycatch.Message()->Get());
        fprintf(stderr,"RUN ERROR: %s\n",*err);fflush(stderr);
        String::Utf8Value exc(trycatch.Exception());
        fprintf(stderr,"RUN ERROR - EXEPTION: %s\n",*exc);fflush(stderr);
        String::Utf8Value st(trycatch.StackTrace());
        fprintf(stderr,"RUN ERROR - STACK: %s\n",*st);fflush(stderr);

        return handle_scope.Close(Boolean::New(false));
    }

    return handle_scope.Close(result);
}
static v8::Handle<v8::Value>jsParse(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    Local<String> source (args[0]->ToString());

    v8::TryCatch trycatch;

    Local<Value> result;
    Local<Script> script = Script::Compile(source); // компилируем
    if (trycatch.HasCaught()) {
        String::Utf8Value err(trycatch.Message()->Get());
        fprintf(stderr,"COMPILE ERROR: %s\n",*err);fflush(stderr);
        String::Utf8Value exc(trycatch.Exception());
        fprintf(stderr,"COMPILE ERROR - EXEPTION: %s\n",*exc);fflush(stderr);
        String::Utf8Value st(trycatch.StackTrace());
        fprintf(stderr,"COMPILE ERROR - STACK: %s\n",*st);fflush(stderr);

        return handle_scope.Close(Boolean::New(false));
    }
    result = script->Run(); // выполняем
    if (trycatch.HasCaught()) {
        String::Utf8Value err(trycatch.Message()->Get());
        fprintf(stderr,"RUN ERROR: %s\n",*err);fflush(stderr);
        String::Utf8Value exc(trycatch.Exception());
        fprintf(stderr,"RUN ERROR - EXEPTION: %s\n",*exc);fflush(stderr);
        String::Utf8Value st(trycatch.StackTrace());
        fprintf(stderr,"RUN ERROR - STACK: %s\n",*st);fflush(stderr);

        return handle_scope.Close(Boolean::New(false));
    }

    return handle_scope.Close(result);
}

static v8::Handle<v8::Value>sha1(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));
    String::Utf8Value   st(args[0]);
/*
    QString blah = QString(QCryptographicHash::hash(*st,QCryptographicHash::Sha1).toHex());
    Local<String> ov = String::New((const char*)rd(blah));
*/
    char    md[50]="NONE";
    SHA1Context sha;
    SHA1Reset(&sha);
    SHA1Input(&sha, (const unsigned char *) *st, st.length());

    if (!SHA1Result(&sha))
    {
        return handle_scope.Close(Boolean::New(false));
    } else {
        sprintf(md,"%08X%08X%08X%08X%08X",
                sha.Message_Digest[0],
                sha.Message_Digest[1],
                sha.Message_Digest[2],
                sha.Message_Digest[3],
                sha.Message_Digest[4]
                );
    }

    return handle_scope.Close(String::New(md));
}

static v8::Handle<v8::Value>rm(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    String::Utf8Value fn(args[0]);

    remove(*fn);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>ren_hh(const Arguments &args)// old new
{
    HandleScope handle_scope;

    if( args.Length() < 2) return handle_scope.Close(Boolean::New(false));

    String::Utf8Value fn1(args[0]);
    String::Utf8Value fn2(args[1]);

    rename(*fn1,*fn2);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>cm(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    String::Utf8Value fn(args[0]);

    mode_t mode = 0755;
    if( args.Length() > 1) mode = args[1]->Int32Value();

    chmod(*fn,mode);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>hmkdir(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    int pr = 0755;
    int ret = 0;


    String::Utf8Value fn(args[0]);
    if( args.Length() > 1) pr = args[1]->Int32Value();

//    mkdir(*fn,pr);
#ifdef _WIN32
    ret = _mkdir(*fn);
#else
    ret = mkdir(*fn,pr);
#endif

    if( ret) return handle_scope.Close(Boolean::New(false));

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>dir(const Arguments &args)
{
    HandleScope handle_scope;

    uv_fs_t r;
    uv_dirent_t d;
    int ret;
    int i = 0;
    char dp[2000] = "./";

    if( args.Length() > 0) {
        String::Utf8Value fnn(args[0]);
        strcpy(dp,*fnn);
    }

    Local<Array> ar = Array::New();

    ret = uv_fs_scandir(uv_default_loop(), &r, dp, 0, 0);
//    fprintf(stderr,"dir  ret %d\n",ret);fflush(stderr);
    if( ret > 0)
    while( uv_fs_scandir_next(&r, &d) != UV_EOF) {
//        fprintf(stderr,"fn:: %s\n",d.name);fflush(stderr);

        Local<Object> obj = Object::New();
        obj->Set(v8::String::NewSymbol("file"),String::New(d.name));
        obj->Set(v8::String::NewSymbol("nm"),String::New(d.name));
        obj->Set(v8::String::NewSymbol("isDir"),Boolean::New(d.type == UV_DIRENT_DIR));
        obj->Set(v8::String::NewSymbol("dir"),Boolean::New(d.type == UV_DIRENT_DIR));
        obj->Set(v8::String::NewSymbol("id"),Boolean::New(d.type == UV_DIRENT_DIR));

        ar->Set(v8::Number::New(i),obj);
        ++i;
    }

    return handle_scope.Close(ar);
}

#ifdef WINDOWS
    #include <direct.h>
    #define GetCurrentDir _getcwd
#else
    #include <unistd.h>
    #define GetCurrentDir getcwd
#endif

#define FILENAME_MAX 2000

struct {
    v8::Persistent<v8::Function> f;
}timer_ad_t;

class tmr_add
{
public:
    v8::Persistent<v8::Function> fnc;
};


static v8::Handle<v8::Value>curdir(const Arguments &args)
{
    HandleScope handle_scope;
    char cCurrentPath[FILENAME_MAX];

    if( !GetCurrentDir(cCurrentPath, sizeof(cCurrentPath))){
        return handle_scope.Close(String::New("./"));
    }

    return handle_scope.Close(String::New(cCurrentPath));
}

void timer_cb(uv_timer_t* h)
{
    HandleScope handle_scope;

//    fprintf(stderr,"ТИМЕР ИДЕТ\n");fflush(stderr);

    tmr_add *dt = (tmr_add *)h->data;

    Handle<v8::Object> global = context->Global();


//    Handle<Value> args[2];

//    fprintf(stderr,"ТИМЕР ИДЕТ 11\n");fflush(stderr);

    dt->fnc->Call(global,0,NULL);

//    fprintf(stderr,"ТИМЕР ИДЕТ 99\n");fflush(stderr);

//    handle_scope.Close();
}

static v8::Handle<v8::Value>timer_prc(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() == 0) return handle_scope.Close(Boolean::New(false));

    uint64_t msec = 1000;

    v8::Local<v8::Function> fnc =  v8::Local<v8::Function>::Cast(args[0]);

    tmr_add *ad =  new tmr_add();
    ad->fnc = v8::Persistent<v8::Function>::New(fnc);

//    o->fnc = v8::Persistent<v8::Function>::New(fnc);

    if( args.Length() > 1) {
        msec = args[1]->Int32Value();
    }

    uv_timer_t* ht = (uv_timer_t*)malloc(sizeof(uv_timer_t));
    uv_timer_init(uv_default_loop(), ht);
//    ht->data = (void*)&v8::Persistent<v8::Function>::New(fnc);
    ht->data = ad;

    uv_timer_start(ht, timer_cb, msec, msec);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value> wclose(const Arguments &args)
{
    HandleScope handle_scope;

    uv_stop(uv_default_loop());
    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>fileWr(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() == 0) return handle_scope.Close(Boolean::New(false));
    String::Utf8Value txt(args[0]);
    int isZ = args.This()->GetInternalField(2)->Int32Value();

    if( !isZ) {
        FILE * f = (FILE *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        fwrite(*txt,1,txt.length(),f);
    }else{
        gzFile f = (gzFile )(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        gzwrite (f, *txt, txt.length());
    }

    return handle_scope.Close(Boolean::New(true));
//    return handle_scope.Close(v8::Number::New(r));
}

static v8::Handle<v8::Value>fileFlush(const Arguments &args)
{
    HandleScope handle_scope;

    int isZ = args.This()->GetInternalField(2)->Int32Value();

    if( !isZ) {
        FILE * f = (FILE *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        fflush(f);
    }else{
        gzFile f = (gzFile )(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        gzflush(f, Z_PARTIAL_FLUSH);
    }

    return handle_scope.Close(Boolean::New(true));
//    return handle_scope.Close(v8::Number::New(r));
}

static v8::Handle<v8::Value>fileGets(const Arguments &args)
{
    HandleScope handle_scope;

    int isZ = args.This()->GetInternalField(2)->Int32Value();

    char b[1000000];
    if( !isZ) {
        FILE * f = (FILE *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        if( fgets(b,1000000,f)) {
            int sl = strlen(b);

            if( b[sl - 1] == 0xd || b[sl - 1] == 0xa) b[sl - 1] = 0;
            if( b[sl - 2] == 0xd || b[sl - 2] == 0xa) b[sl - 2] = 0;

            Local<String> ov = String::New((const char*)b);

            return handle_scope.Close(ov);
        } else {
            return handle_scope.Close(Boolean::New(false));
        }
    }else{
        gzFile f = (gzFile )(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        if( gzgets(f,b,1000000)) {
            int sl = strlen(b);

            if( b[sl - 1] == 0xd || b[sl - 1] == 0xa) b[sl - 1] = 0;
            if( b[sl - 2] == 0xd || b[sl - 2] == 0xa) b[sl - 2] = 0;

            Local<String> ov = String::New((const char*)b);

            return handle_scope.Close(ov);
        } else {
            return handle_scope.Close(Boolean::New(false));
        }
    }

    return handle_scope.Close(Boolean::New(true));
//    return handle_scope.Close(v8::Number::New(r));
}

static v8::Handle<v8::Value>fileClose(const Arguments &args)
{
    HandleScope handle_scope;
    int isZ = args.This()->GetInternalField(2)->Int32Value();
    if( !isZ) {
        FILE * f = (FILE *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        if( f) fclose(f);
    }else {
        gzFile f = (gzFile )(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        if( f) gzclose(f);
    }

    return handle_scope.Close(Boolean::New(true));
}



static v8::Handle<v8::Value>fileHjs(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 2) return handle_scope.Close(Boolean::New(false));
    String::Utf8Value fn(args[0]);
    String::Utf8Value fp(args[1]);

    int isZ = 0;
    if( args.Length() > 2) isZ = 1;

    int id = File_ID;
    Local<Object> obj = Local<Object>::New(file_template->NewInstance());
    void *f = 0;

    if( !isZ) f = fopen(*fn,*fp);
    else f = gzopen(*fn,*fp);

    if( !f) return handle_scope.Close(Boolean::New(false));

//    obj.MakeWeak(o, objectWeakCallback);

    obj->SetInternalField(0, External::New(f));
    obj->SetInternalField(1, Int32::New(id));
    obj->SetInternalField(2, Int32::New(isZ));

    return handle_scope.Close(obj);
}

//----------------------

static v8::Handle<v8::Value>buf_g8(const Arguments &args)
{
    HandleScope handle_scope;

    hBuf * ro = (hBuf *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
//    int32_t rid = args.This()->GetInternalField(1)->Int32Value();
//    if( rid != Buf_ID) return Boolean::New(false);

    int beg=0;

    if( args.Length() > 0) {
        beg = args[0]->Int32Value();
    }

    int8_t *i = (int8_t *)(ro->p + beg);

    return handle_scope.Close(Int32::New(*i));
}

static v8::Handle<v8::Value>buf_g16(const Arguments &args)
{
    HandleScope handle_scope;
    hBuf * ro = (hBuf *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
//    int32_t rid = args.This()->GetInternalField(1)->Int32Value();
//    if( rid != Buf_ID) return Boolean::New(false);

    int beg=0;

    if( args.Length() > 0) {
        beg = args[0]->Int32Value();
    }

    int16_t *i = (int16_t *)(ro->p + beg);

    return handle_scope.Close(Int32::New(*i));
}

static v8::Handle<v8::Value>buf_g32(const Arguments &args)
{
    HandleScope handle_scope;

    hBuf * ro = (hBuf *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
//    int32_t rid = args.This()->GetInternalField(1)->Int32Value();
//    if( rid != Buf_ID) return Boolean::New(false);

    int beg=0;

    if( args.Length() > 0) {
        beg = args[0]->Int32Value();
    }

    int32_t *i = (int32_t *)(ro->p + beg);

    return handle_scope.Close(Int32::New(*i));
}

static v8::Handle<v8::Value>buf_gd(const Arguments &args)
{
    HandleScope handle_scope;

    hBuf * ro = (hBuf *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
//    int32_t rid = args.This()->GetInternalField(1)->Int32Value();
//    if( rid != Buf_ID) return Boolean::New(false);

    int beg=0;

    if( args.Length() > 0) {
        beg = args[0]->Int32Value();
    }

    double *i = (double *)(ro->p + beg);

    return handle_scope.Close(Number::New(*i));
}

static v8::Handle<v8::Value>buf_s8(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    hBuf * ro = (hBuf *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
//    int32_t rid = args.This()->GetInternalField(1)->Int32Value();
//    if( rid != Buf_ID) return Boolean::New(false);

    int beg=0;
    int v = args[0]->Int32Value();
    if( args.Length() > 1) beg = args[1]->Int32Value();

    int8_t *i = (int8_t *)(ro->p + beg);
    *i = (int8_t)v;

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>buf_s16(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    hBuf * ro = (hBuf *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
//    int32_t rid = args.This()->GetInternalField(1)->Int32Value();
//    if( rid != Buf_ID) return Boolean::New(false);

    int beg=0;
    int v = args[0]->Int32Value();
    if( args.Length() > 1) beg = args[1]->Int32Value();

    int16_t *i = (int16_t *)(ro->p + beg);
    *i = (int16_t)v;

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>buf_s32(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    hBuf * ro = (hBuf *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
//    int32_t rid = args.This()->GetInternalField(1)->Int32Value();
//    if( rid != Buf_ID) return Boolean::New(false);

    int beg=0;
    int v = args[0]->Int32Value();
    if( args.Length() > 1) beg = args[1]->Int32Value();

    int32_t *i = (int32_t *)(ro->p + beg);
    *i = (int32_t)v;

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>buf_sd(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    hBuf * ro = (hBuf *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
//    int32_t rid = args.This()->GetInternalField(1)->Int32Value();
//    if( rid != Buf_ID) return Boolean::New(false);

    int beg=0;
    double v = args[0]->NumberValue();
    if( args.Length() > 1) beg = args[1]->Int32Value();

    double *i = (double *)(ro->p + beg);
    *i = (double)v;

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>buf_st(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    hBuf * ro = (hBuf *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
//    int32_t rid = args.This()->GetInternalField(1)->Int32Value();
//    if( rid != Buf_ID) return Boolean::New(false);


    int beg=0;
    String::Utf8Value v(args[0]);
    if( args.Length() > 1) beg = args[1]->Int32Value();

    int len = v.length() + 1;
    if( len > (ro->sz - beg)) return handle_scope.Close(Boolean::New(false));

    strcpy(ro->p + beg,*v);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>buf_par(const Arguments &args)
{
    HandleScope handle_scope;

    hBuf * ro = (hBuf *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

    Local<Object> obj = Object::New();

    obj->Set(v8::String::NewSymbol("sz"),Number::New(ro->sz));
    obj->Set(v8::String::NewSymbol("beg"),Number::New(ro->beg));
    obj->Set(v8::String::NewSymbol("end"),Number::New(ro->end));
    obj->Set(v8::String::NewSymbol("isSlice"),Boolean::New(ro->isSlice));

    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>buf_gt(const Arguments &args)
{
    HandleScope handle_scope;

    hBuf * ro = (hBuf *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

    int beg=0;
    if( args.Length() > 0) beg = args[0]->Int32Value();

    char * pos = ro->p + beg;
/*
    QByteArray ba;

    if( args.Length() > 1) {
        String::Utf8Value cp(args[1]);

        QTextCodec *codec = QTextCodec::codecForName(*cp);
        if (!codec) {
            codec = QTextCodec::codecForName("UTF-8");
        }

        QString s = codec->toUnicode(pos);

        QTextCodec *codec2 = QTextCodec::codecForName("UTF-8");
        ba = codec2->fromUnicode(s);

        pos = ba.data();
    }
*/
    return handle_scope.Close(String::New(pos));
}

int Base64encode(char *encoded, const char *string, int len);

static v8::Handle<v8::Value>buf_b64(const Arguments &args)
{
    HandleScope handle_scope;

    hBuf * ro = (hBuf *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

    int beg=0;
    if( args.Length() > 0) beg = args[0]->Int32Value();

    char * pos = ro->p + beg;
    char *ret = (char *)malloc(50000000);
    Base64encode(ret, pos,ro->sz);

//    QByteArray ba(pos,ro->sz);
//    QByteArray ba2 = ba.toBase64();

    Local<String> out = String::New(ret);
    free(ret);

    return handle_scope.Close(out);
}

static v8::Handle<v8::Value>buf_clr(const Arguments &args)
{
    HandleScope handle_scope;

    hBuf * ro = (hBuf *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
    if( ro->p && !ro->isSlice) {
        free( ro->p);
        ro->p = 0;
        ro->beg = ro->end = ro->sz = 0;
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>buf_slice(const Arguments &args)
{
    HandleScope handle_scope;

    int id = Buf_ID;

    hBuf * ro = (hBuf *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
//    int32_t rid = args.This()->GetInternalField(1)->Int32Value();
//    if( rid != Buf_ID) return Boolean::New(false);

    Persistent<Object> obj = Persistent<Object>::New(buf_template->NewInstance());
    hBuf * o = new hBuf();
    o->isSlice = true;
    int beg=0,end=ro->end;

    if( args.Length() > 0) {
        beg = args[0]->Int32Value();
    }
    if( args.Length() > 1) {
        end = args[1]->Int32Value();
    }

    if( ro->p) {
        o->p = ro->p + beg;
        o->sz = o->end = end - beg;
    }

    obj.MakeWeak(o, objectWeakCallback);

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>buf_uz(const Arguments &args)
{
    HandleScope handle_scope;

    int id = Buf_ID;

    hBuf * ro = (hBuf *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
//    int32_t rid = args.This()->GetInternalField(1)->Int32Value();
//    if( rid != Buf_ID) return Boolean::New(false);

    Persistent<Object> obj = Persistent<Object>::New(buf_template->NewInstance());
    hBuf * o = new hBuf();

    if( args.Length() > 0) {
        int size = args[0]->Int32Value();

        o->p = (char *)malloc(size + 10);
        o->sz = o->end = size;

        uLongf destLen = o->sz;
        uncompress((Bytef *)o->p,&destLen,(Bytef *)(ro->p),ro->end);
        o->end = destLen;
        o->p[o->end] = 0;

    }else {
        int64_t *i64 = (int64_t *)ro->p;
        o->p = (char *)malloc(*i64 + 10);
        o->sz = o->end = *i64 +10;

        uLongf destLen = o->sz;
        uncompress((Bytef *)o->p,&destLen,(Bytef *)(ro->p + 8), ro->end - 8);
        o->end = destLen;
        o->p[o->end] = 0;
    }

    obj.MakeWeak(o, objectWeakCallback);

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>buf_cp(const Arguments &args)
{
    HandleScope handle_scope;

    int id = Buf_ID;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    hBuf * out_o = (hBuf *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
    Local<Object> obj = args[0]->ToObject();
    hBuf * in_o = (hBuf *)(Handle<External>::Cast(obj->GetInternalField(0)))->Value();
    int size = in_o->end - in_o->beg;
    if( !size) {
        size = in_o->sz;
    }
    if( args.Length() > 1) {
        size = args[1]->Int32Value();
    }
    memcpy(out_o->p + out_o->beg,in_o->p + in_o->beg,size);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>buf_z(const Arguments &args)
{
    HandleScope handle_scope;

    int id = Buf_ID;

    hBuf * ro = (hBuf *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
//    int32_t rid = args.This()->GetInternalField(1)->Int32Value();
//    if( rid != Buf_ID) return Boolean::New(false);

    Persistent<Object> obj = Persistent<Object>::New(buf_template->NewInstance());
    hBuf * o = new hBuf();

//    int beg=0,end=0;

    uLong fsz = compressBound(ro->end - ro->beg)+100;

    o->p = (char *)malloc(fsz);
    o->sz = fsz;

    uLongf destLen = fsz-20;
    compress2((Bytef *)(o->p + 8),&destLen,(const Bytef *)(ro->p + ro->beg), ro->end - ro->beg , Z_BEST_COMPRESSION);
    o->end = destLen + 8;
//    o->p[o->end] = 0;
    int64_t *i64 = (int64_t *)o->p;
    *i64 = ro->end - ro->beg;

    //--
    obj.MakeWeak(o, objectWeakCallback);

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>buf(const Arguments &args)
{
    HandleScope handle_scope;

    int id = Buf_ID;
    Persistent<Object> obj = Persistent<Object>::New(buf_template->NewInstance());
    hBuf * o = new hBuf();
    int sz = 1000000;
    if( args.Length() > 0) {
        if( args[0]->IsNumber()) {
            sz = args[0]->Int32Value();
        }

//        if( args[0]->IsString() || args[0]->IsStringObject() ) {
        if( args[0]->IsString()) {

            String::Utf8Value v(args[0]);
            int len = strlen(*v);
            o->p = (char *)malloc(len+5);
            strcpy(o->p,*v);
            o->sz = len;
            o->end = o->sz;

            goto step;
        }
    }


    if( args.Length() > 1) {// shared memory
        String::Utf8Value shmn(args[1]);
        int ret=0;

        if( sz < 0) { // create
            sz = -sz;
            if( !(ret = hh_shmem_create(*shmn,sz,&o->sh))) {
                o->sz = sz;
                o->p = o->sh.pBuf;
            }
//fprintf(stderr,"sss create %d %s ret %d p %x\n",sz, *shmn,ret,o->p);fflush(stderr);
        } else { // open
            if( !(ret = hh_shmem_open(*shmn,sz,&o->sh))) {
                o->sz = sz;
                o->p = o->sh.pBuf;
            }
//fprintf(stderr,"sss open %d %s ret %d p %x\n",sz, *shmn,ret,o->p);fflush(stderr);
        }
    } else {
        if( sz > 0) {
            o->p = (char *)malloc(sz+2);
            memset(o->p,0,sz);
            o->sz = sz;
        }
    }

step:

    obj.MakeWeak(o, objectWeakCallback);

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}


int c_scselect( int sock)
{

    fd_set				scArr;
    timeval				tmW;

    tmW.tv_sec = 0;
    tmW.tv_usec = 10;

#ifdef _WIN32
    scArr.fd_count = 1;
    scArr.fd_array[0] = sock;
    int r = select(0,&scArr,0,0,&tmW);

#else
    FD_ZERO(&scArr);
    FD_SET(sock,&scArr);
    int r = select(sock+1,&scArr,0,0,&tmW);

#endif
    return r;
}

static v8::Handle<v8::Value>scselect(const Arguments &args)
{
    HandleScope handle_scope;

    int sock = (int)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Int32Value();
    if( sock < 0) return handle_scope.Close(Boolean::New(false));
    int listen_flg = (int)args.This()->GetInternalField(2)->Int32Value();

    fd_set				scArr;
    timeval				tmW;

    tmW.tv_sec = 0;
    tmW.tv_usec = 1;

#ifdef _WIN32
    scArr.fd_count = 1;
    scArr.fd_array[0] = sock;
    int r = select(0,&scArr,0,0,&tmW);

#else
    FD_ZERO(&scArr);
    FD_SET(sock,&scArr);
    int r = select(sock+1,&scArr,0,0,&tmW);

#endif

    if( r > 0 ) {
        if( !listen_flg) return handle_scope.Close(Boolean::New(true)); // connect

        // listen

        int					slave;
        struct sockaddr_in	client;
        socklen_t			clilen;

        clilen=sizeof(client);
        slave=::accept(sock,(struct sockaddr *)&client,&clilen);
        if( slave < 0 ) return handle_scope.Close(Boolean::New(false));

//        int optval=1;
//        size_t optlen=sizeof(int);
//        ::setsockopt(slave, SOL_TCP, TCP_NODELAY, (const char *)&optval,optlen);

//        return Boolean::New(true);

        int id = Socket_ID;
//        Persistent<Object> obj = Persistent<Object>::New(sock_template->NewInstance());
        Local<Object> obj = Local<Object>::New(sock_template->NewInstance());

//        return Boolean::New(true);

//        obj.MakeWeak((void *)slave, objectWeakCallback);

        obj->SetInternalField(0, Int32::New(slave));
        obj->SetInternalField(1, Int32::New(id));
        obj->SetInternalField(2, Boolean::New(false));

        return handle_scope.Close(obj);
    }

    return handle_scope.Close(Boolean::New(false));
}
static v8::Handle<v8::Value>scwr(const Arguments &args)
{
    HandleScope handle_scope;

    int sc = (int)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Int32Value();
    if( sc < 0) return handle_scope.Close(Boolean::New(false));

    if( args.Length() == 0) return handle_scope.Close(Boolean::New(false));

//    if( args[0]->IsString() || args[0]->IsStringObject()) {
    if( args[0]->IsString()) {
        String::Utf8Value txt(args[0]);
        int r = send_piece( sc, *txt, 0);
        if( r >= 0) return handle_scope.Close(Boolean::New(true));
        return handle_scope.Close(Boolean::New(false));
    } else {
        Local<Object> obj = args[0]->ToObject();
        Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
        hBuf* o = (hBuf*)field->Value();
        int32_t id = obj->GetInternalField(1)->Int32Value();
        if( id != Buf_ID) return handle_scope.Close(Boolean::New(false));

        int r = send_piece( sc, o->p + o->beg, o->end - o->beg);
        if( r >= 0) return handle_scope.Close(Boolean::New(true));
        return handle_scope.Close(Boolean::New(false));
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>scwr_rest(const Arguments &args)
{
    HandleScope handle_scope;

    int sc = (int)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Int32Value();
    if( sc < 0) return handle_scope.Close(Boolean::New(false));

//fprintf(stderr,"== scwr_rest 0 sc %d\n",sc);fflush(stderr);

    if( args.Length() == 0) return handle_scope.Close(Boolean::New(false));
    String::Utf8Value hd(args[0]);
//fprintf(stderr,"== scwr_rest 1 hd: \n%s\n",*hd);fflush(stderr);

//    send_buf(sc,*hd,hd.length());

//fprintf(stderr,"== scwr_rest 2 sc %d\n",sc);fflush(stderr);

    if( args.Length() > 1) {
        if( args[1]->IsString() || args[1]->IsNumber()) {
            String::Utf8Value bd(args[1]);

    //        char b[100];
    //        sprintf(b,"\nContent-Length: %d\n\n",bd.length());

            char *bb = (char *) malloc(hd.length() + bd.length() + 200);
            sprintf(bb,"%s\nContent-Length: %d\n\n%s",*hd,bd.length(),*bd);
            send_buf(sc,bb,0);

//fprintf(stderr,"send<%s>l: %d\n",bb,bd.length());fflush(stderr);

            free( bb);

    //        send_buf(sc,b,0);
    //        send_buf(sc,*bd,bd.length());
        }else{

            Local<Object> obj = args[1]->ToObject();
            Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
            hBuf* o = (hBuf*)field->Value();

            char *bb = (char *) malloc(hd.length() + o->sz + 200);
//fprintf(stderr,"1> %ld %d %ld\n",bb,o->sz,o->p);fflush(stderr);

            sprintf(bb,"%s\nContent-Length: %d\n\n",*hd,o->sz);//,o->p);
//fprintf(stderr,"2 %s\n",bb);fflush(stderr);
            int bb_l = strlen(bb);
//fprintf(stderr,"3 %d %d\n",bb_l,hd.length());fflush(stderr);
            int f_sz = bb_l + o->sz;
//fprintf(stderr,"4 %d\n",f_sz);fflush(stderr);
            memcpy(bb + bb_l, o->p, o->sz) ;
//fprintf(stderr,"10\n");fflush(stderr);
            send_buf(sc,bb,f_sz);
            free( bb);
/*
            FILE *f = fopen("t1.jpg","wb");
            fwrite(o->p,1,o->sz,f);
            fclose(f);
*/
        }

    } else {
        char *bb = (char *) malloc(hd.length() + 200);
        sprintf(bb,"%s\n\n%s",*hd);
        send_buf(sc,bb,0);
        free( bb);

//        send_buf(sc,"\n\n",0);
    }

    return handle_scope.Close(Boolean::New(true));
}
static v8::Handle<v8::Value>scrd(const Arguments &args)
{
    HandleScope handle_scope;

    int sc = (int)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Int32Value();
    if( sc < 0) return handle_scope.Close(Boolean::New(false));

    int		ret;
    char	*in;

    if( (ret = recv_piece(sc,&in)) < 0 ) {
        hh_closesocket(sc);
        args.This()->SetInternalField(0, Int32::New(-1));

        return handle_scope.Close(Boolean::New(false));
    }

    if( args.Length() == 0) {
        Local<String> str = String::New(in);
        free(in);

        return handle_scope.Close(str);
    }else {// buf return
        int id = Buf_ID;
        Persistent<Object> obj = Persistent<Object>::New(buf_template->NewInstance());
        hBuf * o = new hBuf();
        o->end = o->sz = ret;
        o->p = in;

        obj.MakeWeak(o, objectWeakCallback);

        obj->SetInternalField(0, External::New(o));
        obj->SetInternalField(1, Int32::New(id));
        return handle_scope.Close(obj);
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>scrd_rest(const Arguments &args)
{
    HandleScope handle_scope;

    int sc = (int)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Int32Value();
    if( sc < 0) return handle_scope.Close(Boolean::New(false));

    //---

    int tm_max = 30;
    int szm = 100000;

    if( args.Length() > 0) {
        tm_max = args[0]->Int32Value();
    }

    char *blk = (char *) malloc(szm + 10);
    if(!blk) {
//        hh_closesocket(sc);
        return handle_scope.Close(Boolean::New(false));
    }

    int tot = 0;
    char *p = (char*)blk;
    char *bp = blk;
    int bpn = 0;
    int bd_sz = 0;
    int hd_sz = 0;

    time_t ltm = time(0);
//    time_t ctm = time(0);

    do {
        if( !c_scselect(sc)) {
            if( ( time(0) - ltm) > tm_max) {
                break;
            }

            MSLEEP(100);
            continue;
        }

        ltm = time(0);
        int ret = recv (sc,p,szm-tot,0);

        if(ret == 0 ) { //closed
          break;
        }
        if(ret == -1 ) {
          if (errno!=EINTR){
              break;
          } else {
            continue;
          }
        }
/*
        if( ret > 0) {
            for(int i=0; i < ret; ++i){
                fprintf(stderr,"%c",p[i]);fflush(stderr);
            }
        }
*/
        p += ret;
        tot += ret;

        if( (szm-tot) < (szm/10)) {
            int new_szm = szm *2;
            blk = (char*)realloc(blk,new_szm+10);
            if(!blk) {
//                hh_closesocket(sc);
                return handle_scope.Close(Boolean::New(false));
            }
            p = &blk[tot];

            szm = new_szm;
        }

        blk[tot] = 0;

        // проверки на конец хидера ...

        if( bd_sz){
            if( tot >= bd_sz) {
                break;
            }

            continue;
        }

        char *pp = 0;
        if( bpn == 0 && (pp = strstr(blk,"\015\012\015\012"))) {
            *pp++ = 0;
            *pp++ = 0;
            *pp++ = 0;
            *pp++ = 0;
            bp = pp;
            bpn = pp-blk;

//            fprintf(stderr,"**** 4 %d\n",pp);fflush(stderr);

        }else if( bpn == 0 && (pp = strstr(blk,"\012\012"))) {
            *pp++ = 0;
            *pp++ = 0;
            bp = pp;
            bpn = pp-blk;

//            fprintf(stderr,"**** 2\n");fflush(stderr);
        }

        if( pp) {
            hd_sz = bd_sz = pp - blk;

//            fprintf(stderr,"Опаньки %d\n",hd_sz);fflush(stderr);

            char *szp = strstr(blk,"Content-Length:");
            if( szp) {
                szp = strchr(szp,':');
                ++szp;
                bd_sz += atoi(szp);

            } else {
                break;
            }
        }

        if( bd_sz){
            if( tot >= bd_sz) {
                break;
            }

            continue;
        }

    } while(1);

    blk[tot] = 0;

//    fprintf(stderr,"\nret: \n(%s)\n(%s)\nhl: %d bl: %d tot: %d bd_sz %d hd_sz %d \n",blk,bp,strlen(blk),strlen(bp),tot,bd_sz,hd_sz);fflush(stderr);

/*
    char *hp = blk;
    char *bp = "";

    char *pp = 0;
    if( pp = strstr(blk,"\015\012\015\012")) {
        *pp++ = 0;
        *pp++ = 0;
        *pp++ = 0;
        *pp++ = 0;
        bp = pp;
    }
    if( pp = strstr(blk,"\012\012")) {
        *pp++ = 0;
        *pp++ = 0;
        bp = pp;
    }
*/

//    Local<String> h = String::New(hp);
//    Local<String> b = String::New(bp);

//    Handle<Object> obj = Object::New();

//    obj->Set(v8::String::NewSymbol("h"),String::New(hp));
//    obj->Set(v8::String::NewSymbol("b"),String::New(bp));

    bp = &blk[bpn];

    char *ret_p = bp;
    if( args.Length() > 0){
        if( args[0]->IsString()){
            String::Utf8Value nm(args[0]);
            if( strcmp(*nm,"obj") == 0){
                Handle<Object> obj = Object::New();

                obj->Set(v8::String::NewSymbol("h"),String::New(blk));
                obj->Set(v8::String::NewSymbol("b"),String::New(bp));

                free(blk);

                return handle_scope.Close(obj);
            }
        }

        ret_p = blk;
    }

    Local<String> str = String::New(ret_p);


//    fprintf(stderr,"rcv<%s>LL %d\n",blk,strlen(blk));fflush(stderr);


    free(blk);

    //---

    return handle_scope.Close(str);
}
static v8::Handle<v8::Value>scclose(const Arguments &args)
{
    HandleScope handle_scope;

    int sc = (int)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Int32Value();
    if( sc >= 0) hh_closesocket(sc);

    args.This()->SetInternalField(0, Int32::New(-1));
    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>sendBuf(const Arguments &args)
{
    HandleScope handle_scope;

    int sc = (int)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Int32Value();
    if( sc < 0) return handle_scope.Close(Boolean::New(false));

    if( args.Length() == 0) return handle_scope.Close(Boolean::New(false));

    String::Utf8Value txt(args[0]);
    send_buf( sc, *txt, txt.length());

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>ww(const Arguments &args)
{
    HandleScope handle_scope;

    int sc = (int)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Int32Value();
    if( sc < 0) return handle_scope.Close(Boolean::New(false));

    if( args.Length() == 0) return handle_scope.Close(Boolean::New(false));
    int szm = 100000;
    if( args.Length() > 1) {
        szm = args[1]->Int32Value();
    }

    if( args[0]->IsString()) {
        String::Utf8Value txt(args[0]);

        int r = send_buf( sc, *txt, 0);

        // recive

        char *blk = (char *) malloc(szm + 10);
        if(!blk) {
            return handle_scope.Close(Boolean::New(false));
        }

        int tot = 0;
        char *p = (char*)blk;

        do {
            int ret = recv (sc,p,szm,0);

            if(ret == 0 ) { //closed
              break;
            }
            if(ret == -1 ) {
              if (errno!=EINTR){
                  break;
              } else {
                continue;
              }
            }

            p += ret;
            tot += ret;

        } while(1);

        blk[tot] = 0;
        Local<String> str = String::New(blk);
        free(blk);

        return handle_scope.Close(str);
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>listen(const Arguments &args)
{
    HandleScope handle_scope;

    int socknum = 40000;
    if( args.Length() > 0) {
        socknum = args[0]->Int32Value();
    }

    int sock = -1;
    int ret = BindPassiveSocket(socknum, &sock);
    if( ret < 0 ) return handle_scope.Close(Boolean::New(false));

    int id = Socket_ID;
    Local<Object> obj = Local<Object>::New(sock_template->NewInstance());

//    obj.MakeWeak((void *)sock, objectWeakCallback);

    obj->SetInternalField(0, Int32::New(sock));
    obj->SetInternalField(1, Int32::New(id));
    obj->SetInternalField(2, Boolean::New(true));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>connect(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() <= 0) return handle_scope.Close(Boolean::New(false));
    String::Utf8Value adr(args[0]);

    int sock = hh_connect ((char*)*adr,0);
    if( sock < 0 ) return handle_scope.Close(Boolean::New(false));

    int id = Socket_ID;
    Local<Object> obj = Local<Object>::New(sock_template->NewInstance());

//    obj.MakeWeak((void *)sock, objectWeakCallback);

    obj->SetInternalField(0, Int32::New(sock));
    obj->SetInternalField(1, Int32::New(id));
    obj->SetInternalField(2, Boolean::New(false));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>makeMap(const Arguments &args)
{
    HandleScope handle_scope;

    int id = MapSS_ID;

    if( args.Length() > 0) {
        if( args[1]->IsBoolean()) {
            id = MapSN_ID;
        }else if( args[1]->IsNumber()){
            id = MapSS_ID + args[0]->Int32Value();
        }else if( args[1]->IsString()){
            String::Utf8Value nm(args[1]);
            if( !strcmp(*nm,"SS")) {
                id = MapSS_ID;
            }else if( !strcmp(*nm,"SN")) {
                id = MapSN_ID;
            }else if( !strcmp(*nm,"NS")) {
                id = MapNS_ID;
            }
        }
    }

    Local<Object> obj = Local<Object>::New(map_template->NewInstance());

    if( id == MapSS_ID) {
        HHMapSS_T *o = new HHMapSS_T();
        obj->SetInternalField(0, External::New(o));
    } else if( id == MapSN_ID){
        HHMapSN_T *o = new HHMapSN_T();
        obj->SetInternalField(0, External::New(o));
    } else if( id == MapNS_ID){
        HHMapNS_T *o = new HHMapNS_T();
        obj->SetInternalField(0, External::New(o));
    }

    obj->SetInternalField(1, Int32::New(id));
    obj->SetInternalField(2, External::New(0));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>mapWr(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));
    String::Utf8Value   fn(args[0]);
    char sep = '~';
    if( args.Length() > 1) {
        String::Utf8Value   sp(args[1]);
        sep = (*sp)[0];
    }

    FILE *f = fopen(*fn,"wb");
    if( !f) {
        fprintf(stderr,"ERROR OPEN FILE ( %s ) > %s\n",*fn,strerror(errno));fflush(stderr);

        return handle_scope.Close(Boolean::New(false));
    }

    int id = (Handle<External>::Cast(args.This()->GetInternalField(1)))->Int32Value();

    if( id == MapSS_ID) {
        HHMapSS_T *o = (HHMapSS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        for(HHMapSS_TI it = o->begin(); it != o->end(); ++it)
        {
            fprintf(f,"%s%c%s\n",it->first.c_str(),sep,it->second.c_str());
        }

    }   else if( id == MapSN_ID){
        HHMapSN_T *o = (HHMapSN_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        for(HHMapSN_TI it = o->begin(); it != o->end(); ++it)
        {
            fprintf(f,"%s%c%ld\n",it->first.c_str(),sep,it->second);
        }

    } else if( id == MapNS_ID){
        HHMapNS_T *o = (HHMapNS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        for(HHMapNS_TI it = o->begin(); it != o->end(); ++it)
        {
            fprintf(f,"%ld%c%s\n",it->first,sep,it->second.c_str());
        }
    }

    fclose(f);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>mapRd(const Arguments &args)
{
    HandleScope handle_scope;
    char *b;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));
    String::Utf8Value   fn(args[0]);
    char sep = '~';
    if( args.Length() > 1) {
        String::Utf8Value   sp(args[1]);
        sep = (*sp)[0];
    }

    FILE *f = fopen(*fn,"rb");
    if( !f) return handle_scope.Close(Boolean::New(false));

    int id = (Handle<External>::Cast(args.This()->GetInternalField(1)))->Int32Value();

    b = (char *)malloc(MAX_RD_DB_SIZE + 1);
    if( !b) return handle_scope.Close(Boolean::New(false));

    if( id == MapSS_ID) {
        HHMapSS_T *o = (HHMapSS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        while( fgets(b,MAX_RD_DB_SIZE,f)) {
            int sl = strlen(b);

            if( b[sl - 1] == 0xd || b[sl - 1] == 0xa) b[sl - 1] = 0;
            if( b[sl - 2] == 0xd || b[sl - 2] == 0xa) b[sl - 2] = 0;
            char *p = strchr(b,sep);
            if( !p) continue;

            *p = 0;++p;

            o->insert ( std::pair<std::string, std::string>(b,p) );
        }
    } else if( id == MapSN_ID){
        HHMapSN_T *o = (HHMapSN_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        while( fgets(b,MAX_RD_DB_SIZE,f)) {
            int sl = strlen(b);

            if( b[sl - 1] == 0xd || b[sl - 1] == 0xa) b[sl - 1] = 0;
            if( b[sl - 2] == 0xd || b[sl - 2] == 0xa) b[sl - 2] = 0;
            char *p = strchr(b,sep);
            if( !p) continue;

            *p = 0;++p;
            o->insert ( std::pair<std::string, int64_t>(b,atoll(p)) );
        }

    }    else if( id == MapNS_ID){
        HHMapNS_T *o = (HHMapNS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        while( fgets(b,MAX_RD_DB_SIZE,f)) {
            int sl = strlen(b);

            if( b[sl - 1] == 0xd || b[sl - 1] == 0xa) b[sl - 1] = 0;
            if( b[sl - 2] == 0xd || b[sl - 2] == 0xa) b[sl - 2] = 0;
            char *p = strchr(b,sep);
            if( !p) continue;

            *p = 0;++p;
            o->insert ( std::pair<int64_t,std::string>(atoll(b),p) );
        }

    }

    free(b);

    fclose(f);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>mapClr(const Arguments &args)
{
    HandleScope handle_scope;

    int id = (Handle<External>::Cast(args.This()->GetInternalField(1)))->Int32Value();

    if( id == MapSS_ID) {
        HHMapSS_T *o = (HHMapSS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        o->clear();

        HHMapSS_TI *oil = (HHMapSS_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
        if( oil) delete oil;

    } else if( id == MapSN_ID){
        HHMapSN_T *o = (HHMapSN_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        o->clear();

        HHMapSN_TI *oil = (HHMapSN_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
        if( oil) delete oil;

    } else if( id == MapNS_ID){
        HHMapNS_T *o = (HHMapNS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        o->clear();

        HHMapSN_TI *oil = (HHMapSN_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
        if( oil) delete oil;
    }
    args.This()->SetInternalField(2,External::New(0));

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>mapClose(const Arguments &args)
{
    HandleScope handle_scope;

    int id = (Handle<External>::Cast(args.This()->GetInternalField(1)))->Int32Value();

    if( id == MapSS_ID) {
        HHMapSS_T *o = (HHMapSS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        o->clear();

        HHMapSS_TI *oil = (HHMapSS_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
        if( oil) delete oil;

        delete o;

    } else if( id == MapSN_ID){
        HHMapSN_T *o = (HHMapSN_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        o->clear();

        HHMapSN_TI *oil = (HHMapSN_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
        if( oil) delete oil;

        delete o;

    } else if( id == MapNS_ID){
        HHMapNS_T *o = (HHMapNS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        o->clear();

        HHMapSN_TI *oil = (HHMapSN_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
        if( oil) delete oil;

        delete o;

    }
    args.This()->SetInternalField(2,External::New(0));

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>mapFind(const Arguments &args)
{
    HandleScope handle_scope;

    int id = (Handle<External>::Cast(args.This()->GetInternalField(1)))->Int32Value();

    if( id == MapSS_ID) {
        HHMapSS_T *o = (HHMapSS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        HHMapSS_TI *oi = new HHMapSS_TI();

        if( args.Length() < 1) {
            (*oi) = o->begin();
        }else {
            String::Utf8Value   key(args[0]);
            (*oi) = o->lower_bound(*key);
        }

        HHMapSS_TI *oil = (HHMapSS_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();

        if( oil) delete oil;

        args.This()->SetInternalField(2,External::New(oi));
        return handle_scope.Close(Boolean::New(true));

    } else if( id == MapSN_ID) {
        HHMapSN_T *o = (HHMapSN_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        HHMapSN_TI *oi = new HHMapSN_TI();

        if( args.Length() < 1) {
            (*oi) = o->begin();
        }else {
            String::Utf8Value   key(args[0]);
            (*oi) = o->lower_bound(*key);
        }

        HHMapSN_TI *oil = (HHMapSN_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
        if( oil) delete oil;

        args.This()->SetInternalField(2,External::New(oi));

        return handle_scope.Close(Boolean::New(true));
    } else if( id == MapNS_ID) {
        HHMapNS_T *o = (HHMapNS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        HHMapNS_TI *oi = new HHMapNS_TI();

        if( args.Length() < 1) {
            (*oi) = o->begin();
        }else {
            (*oi) = o->lower_bound(args[0]->IntegerValue());
        }

        HHMapSN_TI *oil = (HHMapSN_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
        if( oil) delete oil;

        args.This()->SetInternalField(2,External::New(oi));

        return handle_scope.Close(Boolean::New(true));
    }

    return handle_scope.Close(Boolean::New(false));
}
static v8::Handle<v8::Value>mapEnd(const Arguments &args)
{
    HandleScope handle_scope;

    int id = (Handle<External>::Cast(args.This()->GetInternalField(1)))->Int32Value();

    if( id == MapSS_ID) {
        HHMapSS_T *o = (HHMapSS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        HHMapSS_TI *oi = new HHMapSS_TI();

        if( args.Length() < 1) {
            (*oi) = o->end();
        }else {
            String::Utf8Value   key(args[0]);
            (*oi) = o->lower_bound(*key);
        }

        HHMapSS_TI *oil = (HHMapSS_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();

        if( oil) delete oil;

        args.This()->SetInternalField(2,External::New(oi));
        return handle_scope.Close(Boolean::New(true));

    } else if( id == MapSN_ID) {
        HHMapSN_T *o = (HHMapSN_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        HHMapSN_TI *oi = new HHMapSN_TI();

        if( args.Length() < 1) {
            (*oi) = o->end();
        }else {
            String::Utf8Value   key(args[0]);
            (*oi) = o->lower_bound(*key);
        }

        HHMapSN_TI *oil = (HHMapSN_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
        if( oil) delete oil;

        args.This()->SetInternalField(2,External::New(oi));

        return handle_scope.Close(Boolean::New(true));
    } else if( id == MapNS_ID) {
        HHMapNS_T *o = (HHMapNS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        HHMapNS_TI *oi = new HHMapNS_TI();

        if( args.Length() < 1) {
            (*oi) = o->end();
        }else {
            (*oi) = o->lower_bound(args[0]->IntegerValue());
        }

        HHMapSN_TI *oil = (HHMapSN_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
        if( oil) delete oil;

        args.This()->SetInternalField(2,External::New(oi));

        return handle_scope.Close(Boolean::New(true));
    }

    return handle_scope.Close(Boolean::New(false));
}


static v8::Handle<v8::Value>mapDel(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    int id = (Handle<External>::Cast(args.This()->GetInternalField(1)))->Int32Value();

    if( id == MapSS_ID) {
        String::Utf8Value   key(args[0]);

        HHMapSS_T *o = (HHMapSS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        o->erase(*key);

    } else if( id == MapSN_ID){
        String::Utf8Value   key(args[0]);

        HHMapSN_T *o = (HHMapSN_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        o->erase(*key);
    } else if( id == MapNS_ID){
//        String::Utf8Value   key(args[0]);

        HHMapNS_T *o = (HHMapNS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        o->erase(args[0]->IntegerValue());
    }

    return handle_scope.Close(Boolean::New(true));
}
static v8::Handle<v8::Value>mapSet(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 2) return handle_scope.Close(Boolean::New(false));

    int id = (Handle<External>::Cast(args.This()->GetInternalField(1)))->Int32Value();

    if( id == MapSS_ID) {
        String::Utf8Value   key(args[0]);
        String::Utf8Value   val(args[1]);

        HHMapSS_T *o = (HHMapSS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        o->insert ( std::pair<std::string, std::string>(*key,*val) );

    } else if( id == MapSN_ID){
        String::Utf8Value   key(args[0]);
        int64_t val   = args[1]->IntegerValue();

        HHMapSN_T *o = (HHMapSN_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        o->insert ( std::pair<std::string, int64_t>(*key,val) );

    } else if( id == MapNS_ID){
        String::Utf8Value   val(args[1]);
        int64_t key = args[0]->IntegerValue();

        HHMapNS_T *o = (HHMapNS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        o->insert ( std::pair<int64_t,std::string>(key,*val) );
    }

    return handle_scope.Close(Boolean::New(true));
}
static v8::Handle<v8::Value>mapGet(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    int id = (Handle<External>::Cast(args.This()->GetInternalField(1)))->Int32Value();

    if( id == MapSS_ID) {
        String::Utf8Value   key(args[0]);

        HHMapSS_T *o = (HHMapSS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        HHMapSS_TI it = o->find(*key);

        if( it == o->end()) {
            return handle_scope.Close(Boolean::New(false));
        }else{
            std::string z = it->second;

            if( args.Length() > 1){
                return handle_scope.Close( parseJS((char*)z.c_str()));
            } else {
                Local<String> ov = String::New( z.c_str());
                return handle_scope.Close(ov);
            }

//            Local<String> ov = String::New( z.c_str());
//            return handle_scope.Close(ov);
        }
    } else if( id == MapSN_ID){
        String::Utf8Value   key(args[0]);

        HHMapSN_T *o = (HHMapSN_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        HHMapSN_TI it = o->find(*key);

        if( it == o->end()) {
            return handle_scope.Close(Boolean::New(false));
        }else{
            int64_t z = it->second;
            return handle_scope.Close(v8::Number::New(z));
        }
    } else if( id == MapNS_ID){
        int64_t key = args[0]->IntegerValue();

        HHMapNS_T *o = (HHMapNS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        HHMapNS_TI it = o->find(key);

        if( it == o->end()) {
            return handle_scope.Close(Boolean::New(false));
        }else{
            std::string z = it->second;

            if( args.Length() > 1){
                return handle_scope.Close( parseJS((char*)z.c_str()));
            } else {
                Local<String> ov = String::New( z.c_str());
                return handle_scope.Close(ov);
            }

//            Local<String> ov = String::New( z.c_str());
//            return handle_scope.Close(ov);
        }
    }

    return handle_scope.Close(Boolean::New(false));
}
static v8::Handle<v8::Value>mapNext(const Arguments &args)
{
    HandleScope handle_scope;

    int id = (Handle<External>::Cast(args.This()->GetInternalField(1)))->Int32Value();

    if( id == MapSS_ID) {
        HHMapSS_T *o = (HHMapSS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        HHMapSS_TI *it = (HHMapSS_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();

        if( !it) handle_scope.Close(Boolean::New(false));
        if( (*it) == o->end() ) return handle_scope.Close(Boolean::New(false));

        std::string f = (*it)->first;
        Local<String> k = String::New( f.c_str());
        std::string s = (*it)->second;

//        Local<String> v = String::New( s.c_str());

        Local<Object> obj = Object::New();

        obj->Set(v8::String::NewSymbol("k"),k);

        if( args.Length() > 1){
//            return handle_scope.Close( parseJS((char*)z.c_str()));
            obj->Set(v8::String::NewSymbol("v"),parseJS((char*)s.c_str()));
        } else {
            Local<String> v = String::New( s.c_str());
            obj->Set(v8::String::NewSymbol("v"),v);
        }

//        obj->Set(v8::String::NewSymbol("v"),v);

        ++(*it);

        return handle_scope.Close(obj);

    } else if( id == MapSN_ID) {
        HHMapSN_T *o = (HHMapSN_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        HHMapSN_TI *it = (HHMapSN_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();

        if( !it) handle_scope.Close(Boolean::New(false));
        if( (*it) == o->end() ) return handle_scope.Close(Boolean::New(false));

        std::string f = (*it)->first;
        Local<String> k = String::New( f.c_str());

        int64_t v = (*it)->second;

        Local<Object> obj = Object::New();

        obj->Set(v8::String::NewSymbol("k"),k);
        obj->Set(v8::String::NewSymbol("v"),v8::Number::New(v));

        ++(*it);

        return handle_scope.Close(obj);
    } else if( id == MapNS_ID) {
        HHMapNS_T *o = (HHMapNS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        HHMapNS_TI *it = (HHMapNS_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();

        if( !it) handle_scope.Close(Boolean::New(false));
        if( (*it) == o->end() ) return handle_scope.Close(Boolean::New(false));

        int64_t k = (*it)->first;
        std::string s = (*it)->second;

//        Local<String> v = String::New( f.c_str());

        Local<Object> obj = Object::New();

        obj->Set(v8::String::NewSymbol("k"),v8::Number::New(k));

        if( args.Length() > 1){
//            return handle_scope.Close( parseJS((char*)z.c_str()));
            obj->Set(v8::String::NewSymbol("v"),parseJS((char*)s.c_str()));
        } else {
            Local<String> v = String::New( s.c_str());
            obj->Set(v8::String::NewSymbol("v"),v);
        }

//        obj->Set(v8::String::NewSymbol("v"),v);

        ++(*it);

        return handle_scope.Close(obj);
    }

    return handle_scope.Close(Boolean::New(false));
}
static v8::Handle<v8::Value>mapPrev(const Arguments &args)
{
    HandleScope handle_scope;

    int id = (Handle<External>::Cast(args.This()->GetInternalField(1)))->Int32Value();

    if( id == MapSS_ID) {
        HHMapSS_T *o = (HHMapSS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        HHMapSS_TI *it = (HHMapSS_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();

        if( !it) handle_scope.Close(Boolean::New(false));
        if( (*it) == o->begin() ) return handle_scope.Close(Boolean::New(false));

        --(*it);

        std::string f = (*it)->first;
        Local<String> k = String::New( f.c_str());
        std::string s = (*it)->second;

//        Local<String> v = String::New( s.c_str());

        Local<Object> obj = Object::New();

        obj->Set(v8::String::NewSymbol("k"),k);

        if( args.Length() > 1){
//            return handle_scope.Close( parseJS((char*)z.c_str()));
            obj->Set(v8::String::NewSymbol("v"),parseJS((char*)s.c_str()));
        } else {
            Local<String> v = String::New( s.c_str());
            obj->Set(v8::String::NewSymbol("v"),v);
        }

//        obj->Set(v8::String::NewSymbol("v"),v);

        return handle_scope.Close(obj);

    } else if( id == MapSN_ID) {
        HHMapSN_T *o = (HHMapSN_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        HHMapSN_TI *it = (HHMapSN_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();

        if( !it) handle_scope.Close(Boolean::New(false));
        if( (*it) == o->begin() ) return handle_scope.Close(Boolean::New(false));

        --(*it);

        std::string f = (*it)->first;
        Local<String> k = String::New( f.c_str());

        int64_t v = (*it)->second;

        Local<Object> obj = Object::New();

        obj->Set(v8::String::NewSymbol("k"),k);
        obj->Set(v8::String::NewSymbol("v"),v8::Number::New(v));

        return handle_scope.Close(obj);
    } else if( id == MapNS_ID) {
        HHMapNS_T *o = (HHMapNS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        HHMapNS_TI *it = (HHMapNS_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();

        if( !it) handle_scope.Close(Boolean::New(false));
        if( (*it) == o->begin() ) return handle_scope.Close(Boolean::New(false));

        --(*it);

        int64_t k = (*it)->first;
        std::string s = (*it)->second;

//        Local<String> v = String::New( f.c_str());

        Local<Object> obj = Object::New();

        obj->Set(v8::String::NewSymbol("k"),v8::Number::New(k));

        if( args.Length() > 1){
//            return handle_scope.Close( parseJS((char*)z.c_str()));
            obj->Set(v8::String::NewSymbol("v"),parseJS((char*)s.c_str()));
        } else {
            Local<String> v = String::New( s.c_str());
            obj->Set(v8::String::NewSymbol("v"),v);
        }

//        obj->Set(v8::String::NewSymbol("v"),v);

        return handle_scope.Close(obj);
    }

    return handle_scope.Close(Boolean::New(false));
}
static v8::Handle<v8::Value>mapSize(const Arguments &args)
{
    HandleScope handle_scope;

    int id = (Handle<External>::Cast(args.This()->GetInternalField(1)))->Int32Value();

    if( id == MapSS_ID) {
        HHMapSS_T *o = (HHMapSS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        int sz = o->size();

        return handle_scope.Close(Integer::New(sz));

    } else if( id == MapSN_ID) {
        HHMapSN_T *o = (HHMapSN_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        int sz = o->size();

        return handle_scope.Close(Integer::New(sz));
    } else if( id == MapNS_ID) {
        HHMapNS_T *o = (HHMapNS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        int sz = o->size();

        return handle_scope.Close(Integer::New(sz));
    }

    return handle_scope.Close(Boolean::New(false));
}
v8::Handle<v8::Value>out_buf(char *b,int sz)
{
//fprintf(stderr,"out_buf:: %s %d\n",b,sz);fflush(stderr);

    int id = Buf_ID;

    HandleScope handle_scope;
    Persistent<Object> obj = Persistent<Object>::New(buf_template->NewInstance());
    hBuf * o = new hBuf();

    o->p = (char *)malloc(sz+2);
    o->sz = sz;
    memcpy(o->p,b,sz);
    o->p[sz] = 0;

    obj.MakeWeak(o, objectWeakCallback);

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));

    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>ld(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));


    String::Utf8Value fn(args[0]);
    //QPluginLoader
//    fprintf(stderr, "Loading.. %s\n", *fn);fflush(stderr);


//    QLibrary myLib(*fn);
//    QLibrary myLib("");

    uv_lib_t *lib = (uv_lib_t*) malloc(sizeof(uv_lib_t));
    if( uv_dlopen(*fn, lib)) {
        fprintf(stderr, "Error: %s\n", uv_dlerror(lib));fflush(stderr);
        free(lib);
        return handle_scope.Close(Boolean::New(false));
    }

//    myLib.load();

//    fprintf(stderr,"*************** (%s) %d\n",*fn,myLib.isLoaded());fflush(stderr);

    typedef v8::Handle<v8::Value> (*MyPrototype)(v8::Persistent<v8::Context> ctx, v8::Handle<v8::Value>(*out_buf)(char *b,int sz));
/*
    MyPrototype myFunction = (MyPrototype) myLib.resolve("init");

    if( myFunction) {
//        fprintf(stderr,"FIND!!\n");fflush(stderr);

        return handle_scope.Close(myFunction(context,out_buf));
    }
*/
    MyPrototype myFunction;
    if (uv_dlsym(lib, "init", (void **) &myFunction)) {
        fprintf(stderr, "dlsym error: %s\n", uv_dlerror(lib));fflush(stderr);
        return handle_scope.Close(Boolean::New(false));
    }
    return handle_scope.Close(myFunction(context,out_buf));

//    return handle_scope.Close(Boolean::New(false));
}

static time_t tm = 0;
static time_t tm_clone = 0;
static char par[2000]= "";

void infined_t_set(int sc,char *p)
{
    if( p == 0) {
        par[0] = 0;
    }else if( p[0] == 0){
        par[0] = 0;
    }else {
        strcpy(par,"-R");
        strcat(par,p);
    }

    if( sc == 0) {
        tm = 0;
    }else{
        tm = time(0) + sc;
    }
}

void pcloning()
{
    char *a[200];
    int ac = argc_;
    memset(a,0,sizeof(a));
    int f_ret = 0;

    int i = 0;
    for(i=0; i < argc_; ++i) {
//                    fprintf(stderr,"%s\n",argv_[i]);fflush(stderr);
        if( !strcmp(argv_[i],"-CLONE")){
            f_ret = 1;
        }
        a[i] = argv_[i];
    }

    if( !f_ret) {
        a[i] = "-CLONE";
        ac++;
    }

//    int i = 0;
//    for(i=0; i < argc_; ++i) {
////                    fprintf(stderr,"%s\n",argv_[i]);fflush(stderr);
//        if( !strcmp(argv_[i],"-CLONE")){
//            f_ret = 1;
//        }
//        a[i] = argv_[i];
//    }

//    if( !f_ret) {
//        a[i] = "-CLONE";
//        ac++;
//    }

    __pid_t npid = fork();

    if( !npid) {
//                    fprintf(stderr,"\nIn new execv PROCESS! %s %s\n",prs,par);fflush(stderr);
/*
        if( args.Length() > 0 ){
            ::close(0);
            ::close(1);
            ::close(2);
        }

        if( args.Length() > 1 ) {
            String::Utf8Value d(args[1]);
            chdir(*d);
        }
*/
        setsid();
        execvp(a[0],a);
//        setsid();
    }
/*
    if( args.Length() > 0 ) {
        for(int i=0; a[i] && i < 200; ++i){
            free(a[i]);
        }
    }
*/
}

void ktThread(void *arg) {
    while( !quit_flg) {
        time_t tml = tm;
        time_t tc = time(0);

        if( tm_clone && tm_clone < tc){
            pcloning();

            tm_clone = 0;
        }

        if( tml == 0) {
            MSLEEP(150);
            continue;
        }
        if( tml < tc){
            if( par[0] != 0) {
//                char prs[3000] = "Нечто";
//                fprintf(stderr,"\nexecv PROCESS! %s %s\n",prs,par);fflush(stderr);

                char *a[200];
                int ac = argc_;
                memset(a,0,sizeof(a));
                int f_ret = 0;

                int i = 0;
                for(i=0; i < argc_; ++i) {
//                    fprintf(stderr,"%s\n",argv_[i]);fflush(stderr);
                    if( !strncmp(argv_[i],"-R",2)){
                        a[i] = par;
                        f_ret = 1;
                        continue;
                    }
                    a[i] = argv_[i];
                }

                if( !f_ret) {
                    a[i] = par;
                    ac++;
                }
/*
                for(int j=0; j < ac; ++j) {
                    fprintf(stderr,"%s\n",a[j]);fflush(stderr);
                }
*/

                if( !fork()) {
//                    fprintf(stderr,"\nIn new execv PROCESS! %s %s\n",prs,par);fflush(stderr);
                    execvp(a[0],a);
//                    fprintf(stderr,"\nIn new execv PROCESS! %s %s\n",prs,par);fflush(stderr);
                    return;
                }


            }

            fprintf(stderr,"\nKILLED BY TIMEOUT! %d %d \n",tml , tc);fflush(stderr);
            _exit(-1);
            return;
        }
        MSLEEP(150);
    }
}

static v8::Handle<v8::Value>pclone(const Arguments &args)
{
    HandleScope handle_scope;

    char *a[200];
    int ac = argc_;
    memset(a,0,sizeof(a));
    int f_ret = 0;

    if( args.Length() > 0 ) {
        if( !args[0]->IsArray()) return handle_scope.Close(Boolean::New(false));

        Handle<Value> v = args[0];
        v8::Handle<v8::Array> obj = v8::Handle<v8::Array>::Cast(v);

        int length = obj->Get(v8::String::New("length"))->ToObject()->Uint32Value();


        for(int i = 0; i < length; i++)
        {
            String::Utf8Value s(obj->Get(i));
//            fprintf(stderr,"pclone %s %d %d\n",*s,s.length(),length);fflush(stderr);
            char* p = (char*)malloc(s.length()+ 5);
            strcpy(p,*s);
            a[i] = p;
        }

    }else{
        int i = 0;
        for(i=0; i < argc_; ++i) {
//                    fprintf(stderr,"%s\n",argv_[i]);fflush(stderr);
            if( !strcmp(argv_[i],"-CLONE")){
                f_ret = 1;
            }
            a[i] = argv_[i];
        }

        if( !f_ret) {
            a[i] = "-CLONE";
            ac++;
        }
    }

//    int i = 0;
//    for(i=0; i < argc_; ++i) {
////                    fprintf(stderr,"%s\n",argv_[i]);fflush(stderr);
//        if( !strcmp(argv_[i],"-CLONE")){
//            f_ret = 1;
//        }
//        a[i] = argv_[i];
//    }

//    if( !f_ret) {
//        a[i] = "-CLONE";
//        ac++;
//    }

    __pid_t npid = fork();

    if( !npid) {
//                    fprintf(stderr,"\nIn new execv PROCESS! %s %s\n",prs,par);fflush(stderr);

        if( args.Length() > 0 ){
            ::close(0);
            ::close(1);
            ::close(2);
        }

        if( args.Length() > 1 ) {
            String::Utf8Value d(args[1]);
            chdir(*d);
        }

        setsid();
        execvp(a[0],a);
//        setsid();
    }

    if( args.Length() > 0 ) {
        for(int i=0; a[i] && i < 200; ++i){
            free(a[i]);
        }
    }

    return handle_scope.Close(Number::New(npid));
}

static v8::Handle<v8::Value>pid_fnc(const Arguments &args)
{
    HandleScope handle_scope;

    pid_t id = getpid();

    return handle_scope.Close( Number::New(id));
}

static v8::Handle<v8::Value>kt(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() > 0) {
        if( args.Length() > 1) {
            String::Utf8Value   s(args[1]);
            infined_t_set( args[0]->Int32Value(),*s);
        } else {
            infined_t_set( args[0]->Int32Value(),0);
        }
    } else {
        infined_t_set( 0,(char *)0);
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>ct_fnc(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() > 0) {
        int tt = args[0]->IntegerValue();
        tm_clone = time(0) + tt;
    } else {
        tm_clone = 0;
    }

    return handle_scope.Close(Boolean::New(true));
}

#define BUF_INT_SIZE 50000
static char	b[BUF_INT_SIZE] = "";
static char	b_in[BUF_INT_SIZE] = "";
static int bin_flg = 0;

void getsTrun(void* p)
{
    while( gets(b) && !quit_flg) {
        while( bin_flg) {
            if( quit_flg) return;
            MSLEEP(20);
        }

        strcpy(b_in,b);
        bin_flg = 1;

//       fprintf(stderr,"==============%s\n",b);fflush(stderr);
    }
}
static v8::Handle<v8::Value>hread(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() > 0) {
        String::Utf8Value fn(args[0]);

        int fsz=0;
        char *fb = get_file_text_utf8(*fn,&fsz);
        if( !fb) return handle_scope.Close(Boolean::New(false));

        if( args.Length() > 1) {// fn,buf
            int id = Buf_ID;
            Persistent<Object> obj = Persistent<Object>::New(buf_template->NewInstance());
            hBuf * o = new hBuf();
            o->end = o->sz = fsz;
            o->p = fb;

            obj.MakeWeak(o, objectWeakCallback);

            obj->SetInternalField(0, External::New(o));
            obj->SetInternalField(1, Int32::New(id));
            return handle_scope.Close(obj);
        }else {
            Local<String> ov = String::New((const char*)fb);
            free(fb);

            return handle_scope.Close(ov);
        }
    }else {
//        return handle_scope.Close(Boolean::New(false));
///*
//        v8::V8::IdleNotification();

        if( bin_flg) return handle_scope.Close(Boolean::New(false));

        Local<String> ov = String::New((const char*)b_in);
        bin_flg = 0;

        return handle_scope.Close(ov);
//*/
    }

    return handle_scope.Close(Boolean::New(false));
}
static v8::Handle<v8::Value>wr(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

//    if( args[0]->IsString() || args[0]->IsStringObject()) {
    if( args[0]->IsString() || args[0]->IsNumber()) {
        String::Utf8Value txt(args[0]);

        if( args.Length() > 1) {//file
            String::Utf8Value fn(args[1]);
            FILE *f = fopen(*fn,"wb");
            if( f) {
                fwrite(*txt,1,strlen(*txt),f);
                fclose(f);
            }
        }else {//stdout
            fwrite(*txt,1,strlen(*txt),stdout);
            fflush(stdout);
        }
    } else {
/*
        hBuf * ro = (hBuf *)(Handle<External>::Cast(args[0]))->Value();
        int32_t rid = args[0]->Int32Value();
        if( rid != Buf_ID) return Boolean::New(false);
*/
        Local<Object> obj = args[0]->ToObject();
        Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
        hBuf* o = (hBuf*)field->Value();
        int32_t id = obj->GetInternalField(1)->Int32Value();
        if( id != Buf_ID) return handle_scope.Close(Boolean::New(false));


        char *p = o->p + o->beg;
        int sz = o->end - o->beg;

        if( args.Length() > 1) {//file
            String::Utf8Value fn(args[1]);
            FILE *f = fopen(*fn,"wb");
            if( f) {
                fwrite(p,1,sz,f);
                fclose(f);
            }
        }else {//stdout
            fwrite(p,1,sz,stdout);
            fflush(stdout);
        }
    }

    return handle_scope.Close(Boolean::New(true));
}
static v8::Handle<v8::Value>proc(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() == 0) return handle_scope.Close(Boolean::New(false));
    String::Utf8Value exe(args[0]);


    int id = Proc_ID;
//    Persistent<Object> obj = Persistent<Object>::New(proc_template->NewInstance());
    Local<Object> obj = Local<Object>::New(proc_template->NewInstance());
    hhProcess * o = new hhProcess();
    o->start(*exe);

    run_process = 1;

//    obj.MakeWeak(o, objectWeakCallback);

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>prcClose(const Arguments &args)
{
    HandleScope handle_scope;

    hhProcess * ro = (hhProcess *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
    ro->close();

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>prcIsActiv(const Arguments &args)
{
    HandleScope handle_scope;

    hhProcess * ro = (hhProcess *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
    return handle_scope.Close(Boolean::New(ro->isActiv()));
}

static v8::Handle<v8::Value>prcRd(const Arguments &args)
{
    HandleScope handle_scope;

    hhProcess * ro = (hhProcess *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

    char b[100000];
    int r = ro->readLine(b,100000);

    if( r < 0 ) {
        return handle_scope.Close(Boolean::New(false));
    }

    return handle_scope.Close(String::New(b));
}

static v8::Handle<v8::Value>prcCanRd(const Arguments &args)
{
    HandleScope handle_scope;

    hhProcess * ro = (hhProcess *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
    int r = ro->isRd();

    if( !r) {
        return handle_scope.Close(Boolean::New(false));
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>prcWr(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() == 0) return handle_scope.Close(Boolean::New(false));

    hhProcess * ro = (hhProcess *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
    String::Utf8Value txt(args[0]);

    int r = ro->write(*txt);

    if( r < 0) return handle_scope.Close(Boolean::New(false));

    return handle_scope.Close(Boolean::New(true));
//    return handle_scope.Close(v8::Number::New(r));
}

static v8::Handle<v8::Value>db(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 6) return handle_scope.Close(Boolean::New(false));

    String::Utf8Value drv(args[0]);
    String::Utf8Value dbname(args[1]);
    String::Utf8Value host(args[2]);
    String::Utf8Value basenm(args[3]);
    String::Utf8Value user(args[4]);
    String::Utf8Value pass(args[5]);

/*
    String::Utf8Value port = "";

    if( args.Length() > 6) {
        port = (int)(args[6]->NumberValue());
    }
*/
    int pos = 0;
    if( dbname.length() == 0) {
        pos = 0;
        if( pgdb[0].con) {
            PQfinish(pgdb[0].con);
            pgdb[0].con = 0;
        }
    }else{
        int i;
        for(i = 1; i < PGDB_T_MAX; ++i){
            if( !pgdb[i].con) {
                pos = i;
                break;
            }
        }
        if( i >= PGDB_T_MAX) {
            return handle_scope.Close(Boolean::New(false));
        }
    }

    PGconn * conn = PQsetdbLogin(*host,
                         NULL,
                         NULL,
                         NULL,
                         *basenm,
                         *user,
                         *pass);

    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf(stderr, "Connection to database failed: %s\n",
                PQerrorMessage(conn));
        return handle_scope.Close(Boolean::New(false));
    }

//    int pv = PQprotocolVersion(conn);
//     fprintf(stderr, "Protocol version: %d\n",pv);

    pgdb[pos].con = conn;
    strcpy( pgdb[pos].nm,*dbname);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>dbclose(const Arguments &args)
{
    HandleScope handle_scope;

//    int pos = 0;

    if( args.Length() > 0 ) {
        String::Utf8Value dbname(args[0]);

        if( dbname.length() == 0) {
//            pos = 0;
            if( pgdb[0].con) {
                PQfinish(pgdb[0].con);
                pgdb[0].con = 0;
            }
        }else{
            int i;
            for(i = 1; i < PGDB_T_MAX; ++i){
                if( strcmp(pgdb[i].nm,*dbname)) continue;

                if( !pgdb[i].con) {
//                    pos = i;

                    PQfinish(pgdb[i].con);
                    pgdb[i].con = 0;
                }
                break;
            }
            if( i >= PGDB_T_MAX) {
                return handle_scope.Close(Boolean::New(false));
            }
        }
    }

    return handle_scope.Close(Boolean::New(true));
}

//typedef struct {


// }SqlAddT;

static v8::Handle<v8::Value>sql_func(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 2 ) return handle_scope.Close(Boolean::New(false));
    String::Utf8Value dbname(args[0]);
    String::Utf8Value sqls(args[1]);
    int flg_exec = 1;

    int pos = 0;

    if( dbname.length() == 0) {
        pos = 0;
        if( !pgdb[0].con) {
            return handle_scope.Close(Boolean::New(false));
        }
    }else{
        int i;
        for(i = 1; i < PGDB_T_MAX; ++i){
            if( strcmp(pgdb[i].nm,*dbname)) continue;

            if( !pgdb[i].con) {
                return handle_scope.Close(Boolean::New(false));
            }
            pos = i;
            break;
        }
        if( i >= PGDB_T_MAX) {
            return handle_scope.Close(Boolean::New(false));
        }
    }

    //--
    char *sql = (char *)malloc(sqls.length() + 2);
    strcpy(sql,*sqls);

    //--

    PGresult   *res = (PGresult   *)0;
    if( strchr(*sqls,'$') != NULL || strchr(*sqls,':') != NULL ) flg_exec = 0;
//flg_exec = 0;
//    fprintf(stderr, "flg_exec %d\n",flg_exec);fflush(stderr);

    if( args.Length() > 2 ) {//exec
//        fprintf(stderr, "args.Length() > 2!!! pos %d\n",pos);fflush(stderr);

        res = PQexec( pgdb[pos].con, *sqls);
        if (PQresultStatus(res) == PGRES_FATAL_ERROR)
        {
            fprintf(stderr, "PQexec <%s> failed: %s %d\n",*sqls,PQerrorMessage(pgdb[pos].con),PQresultStatus(res));fflush(stderr);
            PQclear(res);
            return handle_scope.Close(Boolean::New(false));
        }
//        PQclear(res);
    }else{

    }

    //--

    int id = PSql_ID;
    Persistent<Object> obj = Persistent<Object>::New(sql_template->NewInstance());

    SqlAdd *par = new SqlAdd();

    par->con = pos;
    par->flg_exec = flg_exec;
    par->res = res;
    par->sql = sql;
    par->stnm[0] = 0;
    par->pos = 0;

    obj.MakeWeak(par, objectWeakCallback);

//    PGresult   * o = (PGresult   * )((Handle<External>::Cast(obj->GetInternalField(0)))->Value());

    obj->SetInternalField(0, External::New(par));
    obj->SetInternalField(1, Int32::New(id));
/*
    obj->SetInternalField(2, External::New(pgdb[pos].con));
    obj->SetInternalField(3, External::New(sql));
    obj->SetInternalField(4, Int32::New(0)); // счетчик записей для next
    obj->SetInternalField(5, Int32::New(flg_exec)); // исполнять без обработки параметров(no prepare)?
    obj->SetInternalField(6, External::New(0)); //имя сохраненной сущности
*/

//    obj.Dispose();
//    fprintf(stderr, "sql %d\n",99);fflush(stderr);

    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>close(const Arguments &args)
{
    HandleScope handle_scope;

    SqlAdd   * p = (SqlAdd   * )((Handle<External>::Cast(args.This()->GetInternalField(0)))->Value());

    if( p->res) {
        PQclear(p->res);
        p->res = 0;
    }
    if( p->sql) {

        free(p->sql);
        p->sql = 0;
    }

    if( p->stnm[0] != 0) {
        char b[300];
        sprintf(b,"DEALLOCATE %s",p->stnm);
        PGresult *res = PQexec( pgdb[p->con].con, b);
        if( PQresultStatus(res) == PGRES_FATAL_ERROR) {
            fprintf(stderr, "PQexec <%s> failed: %s %d\n",b,PQerrorMessage(pgdb[p->con].con),PQresultStatus(res));fflush(stderr);
        }
        PQclear(res);

    }
    p->stnm[0] = 0;


    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>sqlerr(const Arguments &args)
{
    HandleScope handle_scope;

    SqlAdd   * p = (SqlAdd   * )((Handle<External>::Cast(args.This()->GetInternalField(0)))->Value());

//    PGconn *  conn = (PGconn *  )((Handle<External>::Cast(args.This()->GetInternalField(2)))->Value());

    return handle_scope.Close(String::New( PQerrorMessage(pgdb[p->con].con)));
}

static v8::Handle<v8::Value>rows(const Arguments &args)
{
    HandleScope handle_scope;

    SqlAdd   * p = (SqlAdd   * )((Handle<External>::Cast(args.This()->GetInternalField(0)))->Value());

//    PGresult   * res = (PGresult   * )((Handle<External>::Cast(args.This()->GetInternalField(0)))->Value());
    if( !p->res) return handle_scope.Close(Boolean::New(false));

//    return handle_scope.Close(Integer::New(PQntuples(p->res)));
    return handle_scope.Close(Integer::New( atol(PQcmdTuples(p->res))));
}

static v8::Handle<v8::Value>next(const Arguments &args)
{
    HandleScope handle_scope;

    SqlAdd   * p = (SqlAdd   * )((Handle<External>::Cast(args.This()->GetInternalField(0)))->Value());

    if( !p->res) return handle_scope.Close(Boolean::New(false));

    int nFields = PQnfields(p->res);
    int nTuples = PQntuples(p->res);

    int pos = p->pos;
    if(pos >= nTuples) return handle_scope.Close(Boolean::New(false));

    //--

    Local<Object> obj = Object::New();

    for(int i=0; i < nFields; ++i) {

        char    *fnb = PQfname(p->res, i);
        int len = PQgetlength(p->res,pos,i);
        char * v = PQgetvalue(p->res,pos,i);
        Oid type = PQftype(p->res, i);
        int format = PQfformat(p->res, i);

        switch (type) {
        case QBOOLOID:
            obj->Set(v8::String::NewSymbol(fnb),Boolean::New((bool)(v[0] == 't')));

            break;
        case QINT8OID:
        case QINT2OID:
        case QINT4OID:
        case QNUMERICOID:
            obj->Set(v8::String::NewSymbol(fnb),Integer::New(atol(v)));
            break;
        case QFLOAT4OID:
        case QFLOAT8OID:
//            type = QVariant::Double;
            obj->Set(v8::String::NewSymbol(fnb),Number::New(atof(v)));
            break;
/*
        case QABSTIMEOID:
        case QRELTIMEOID:
        case QDATEOID:
//            type = QVariant::Date;
            break;
        case QTIMEOID:
        case QTIMETZOID:
//            type = QVariant::Time;
            break;
        case QTIMESTAMPOID:
        case QTIMESTAMPTZOID:
//            type = QVariant::DateTime;
            break;
*/
        case QBYTEAOID:
            {
///*
            size_t to_length;
            char * vv = (char * )PQunescapeBytea((const unsigned char *)v, &to_length);

            char *p = (char *)malloc(to_length + 10);

            memcpy(p,vv,to_length );
            p[to_length] = 0;

            PQfreemem(vv);

            Persistent<Object> ob = Persistent<Object>::New(buf_template->NewInstance());
            hBuf * o = new hBuf();
            o->end = to_length;
            o->sz = o->end + 1;
            o->p = p;
//*/
/*
            //--

            char *p = (char *)malloc(len + 10);
            memcpy(p,v,len );
            p[len] = 0;

            Persistent<Object> ob = Persistent<Object>::New(buf_template->NewInstance());
            hBuf * o = new hBuf();
            o->end = len;
            o->sz = o->end + 1;
            o->p = p;

            //--
*/
            ob.MakeWeak(o, objectWeakCallback);

            ob->SetInternalField(0, External::New(o));
            ob->SetInternalField(1, Int32::New(Buf_ID));

            obj->Set(v8::String::NewSymbol(fnb),ob);
//            ob.Dispose();

            }
            break;
        default:
            obj->Set(v8::String::NewSymbol(fnb),String::New((char*)v));

            break;
        }
    }

    ++pos;
    p->pos = pos;

//    args.This()->SetInternalField(4, Int32::New(pos));

    return handle_scope.Close(obj);
}

static int stnm_cnt = 0;
#define exec_par_max 50

static v8::Handle<v8::Value>exec(const Arguments &args)
{
    HandleScope handle_scope;

//    fprintf(stderr, "exec 00\n");fflush(stderr);

    SqlAdd   * p = (SqlAdd   * )((Handle<External>::Cast(args.This()->GetInternalField(0)))->Value());

    if( p->res) {
        PQclear(p->res);
        p->res = NULL;
    }

//    PGresult   * res = (PGresult   * )((Handle<External>::Cast(args.This()->GetInternalField(0)))->Value());
//    if( res) PQclear(res);
//    args.This()->SetInternalField(0, Int32::New(0));

//    fprintf(stderr, "exec 0\n");fflush(stderr);
//    p->stnm[0] = 0;
    p->pos = 0;

//    PGconn * conn = (PGconn   * )((Handle<External>::Cast(args.This()->GetInternalField(2)))->Value());
//    char   * sql = (char   * )((Handle<External>::Cast(args.This()->GetInternalField(3)))->Value());

//    fprintf(stderr, "exec 3 %s %ld %d\n",p->sql,pgdb[p->con].con,p->flg_exec);fflush(stderr);

    PGresult   * nres = 0;
//    if( p->flg_exec || args.Length() == 0 ) {
    if( args.Length() == 0 ) {
//        fprintf(stderr, "exec 3.5\n");fflush(stderr);
        nres = PQexec( pgdb[p->con].con, p->sql);
//        fprintf(stderr, "exec 4\n");fflush(stderr);
        if (PQresultStatus(nres) == PGRES_FATAL_ERROR)
        {
            fprintf(stderr, "PQexec <%s> failed: %s %d\n",p->sql,PQerrorMessage(pgdb[p->con].con),PQresultStatus(nres));fflush(stderr);
            PQclear(nres);
            return handle_scope.Close(Boolean::New(false));
        }
        p->res = nres;
    }else{

//        Local<String::Utf8Value> pr[exec_par_max];// = String::New((const char*)fb);
        char *pr_v[exec_par_max];
        int   pr_l[exec_par_max];
        Oid   pr_t[exec_par_max];

        for(int i=0; i < args.Length(); ++i){
            if( args[i]->IsObject() ) {

                Local<Object> obj = args[i]->ToObject();
                Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
                void* o = field->Value();
                hBuf * bu = (hBuf * )o;
                int32_t id = obj->GetInternalField(1)->Int32Value();
                if( id != Buf_ID) return handle_scope.Close(Boolean::New(false));

                pr_v[i] = bu->p;
                pr_l[i] = bu->sz;
                pr_t[i] = QBYTEAOID;

                continue;
            }

//            char *ptr = "";

//            Local<String::Utf8Value> t = new String::Utf8Value(args[i]);
//            String::Utf8Value t = new String::Utf8Value(args[i]);

            String::Utf8Value t(args[i]);

            char *ptr = (char *)malloc(t.length() + 5);
            strcpy(ptr,*t);

            pr_v[i] = ptr;
            pr_l[i] = t.length();

//            pr_l[i] = 0;

            if( args[i]->IsBoolean() ) pr_t[i] = QBOOLOID;
            if( args[i]->IsNumber() ) pr_t[i] = QINT8OID;//QFLOAT8OID;
            if( args[i]->IsString() ) pr_t[i] = NULL;

        }

//fprintf(stderr, "10-\n");fflush(stderr);

        if( p->stnm[0] == 0){//prepare
            sprintf(p->stnm,"ej_stmt_%d",stnm_cnt++);
//            fprintf(stderr, "exec PREPARE %s\n",p->stnm);fflush(stderr);

            PGresult *r_p = PQprepare(pgdb[p->con].con,
                                p->stnm,
                                p->sql,
                                args.Length(),
                                pr_t);
            if (PQresultStatus(r_p) == PGRES_FATAL_ERROR) {
                fprintf(stderr, "PQexec <%s> failed: %s %d\n",p->sql,PQerrorMessage(pgdb[p->con].con),PQresultStatus(r_p));fflush(stderr);
                p->stnm[0] = 0;
            }
            PQclear(r_p);
        }
//fprintf(stderr, "11-\n");fflush(stderr);

        //-- working

        if( p->stnm[0] != 0){ //working
//            fprintf(stderr, "exec working 1\n");fflush(stderr);
//fprintf(stderr, "12- %s\n",p->stnm);fflush(stderr);

            PGresult *res = PQexecPrepared(pgdb[p->con].con,
                                     p->stnm,
                                     args.Length(),
                                     pr_v,
                                     pr_l,
                                     NULL,
                                     NULL);
//fprintf(stderr, "13-\n");fflush(stderr);

            if( PQresultStatus(res) == PGRES_FATAL_ERROR) {
                fprintf(stderr, "PQexec <%s> failed: %s %d\n",p->sql,PQerrorMessage(pgdb[p->con].con),PQresultStatus(res));fflush(stderr);
                PQclear(res);
            }else{
                p->res = res;
            }

        }

//        fprintf(stderr, "14-\n");fflush(stderr);
        // clear
        for(int i=0; i < args.Length(); ++i){
            if( args[i]->IsObject() ) continue;

            free(pr_v[i]);
        }
//        fprintf(stderr, "15-\n");fflush(stderr);

    }

//    fprintf(stderr, "exec 99\n");fflush(stderr);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>prepare(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));
    String::Utf8Value sqls(args[0]);

    SqlAdd   * p = (SqlAdd   * )((Handle<External>::Cast(args.This()->GetInternalField(0)))->Value());

    if( p->res) {
        PQclear(p->res);
        p->res = 0;
    }
    if( p->sql) {
        free(p->sql);
        p->sql = 0;
    }

    if( p->stnm[0] != 0) {
        char b[300];
        sprintf(b,"DEALLOCATE %s",p->stnm);
        PGresult *res = PQexec( pgdb[p->con].con, b);
        if( PQresultStatus(res) == PGRES_FATAL_ERROR) {
            fprintf(stderr, "PQexec <%s> failed: %s %d\n",b,PQerrorMessage(pgdb[p->con].con),PQresultStatus(res));fflush(stderr);
        }
        PQclear(res);
    }
    p->stnm[0] = 0;

    p->pos = 0;

    char *sql = (char *)malloc(sqls.length() + 2);
    strcpy(sql,*sqls);
    p->sql = sql;

    int flg_exec = 1;
//    if( strchr(*sqls,'$') != NULL || strchr(*sqls,':') != NULL ) flg_exec = 0;
    p->flg_exec = flg_exec;

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>bind(const Arguments &args)
{
    HandleScope handle_scope;
    return handle_scope.Close(Boolean::New(true));
}


static v8::Handle<v8::Value>lock_close(const Arguments &args)
{
    HandleScope handle_scope;

    sem_t * instance = (sem_t *)args.This()->GetAlignedPointerFromInternalField(0);
    char * p = (char *)args.This()->GetAlignedPointerFromInternalField(1);

    if( !instance) goto END;

    sem_close(instance);
    sem_unlink( p);

//    if( args.Length() > 0) {
//        String::Utf8Value sn(args[0]);
//        sem_unlink( *sn);
//    }

    if( p) free(p);

    args.This()->SetAlignedPointerInInternalField(0, 0);
    args.This()->SetAlignedPointerInInternalField(1, 0);

END:
    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>lock_fnc(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));
    String::Utf8Value sn(args[0]);
    char *p = (char *)malloc(sn.length()+ 5);
    strcpy( p,*sn);

    int id = Lock_ID;
    Local<Object> obj = Local<Object>::New(lock_template->NewInstance());

    sem_t *sem = sem_open(*sn, O_CREAT | O_EXCL, 0777, 0);

    if( !sem) return handle_scope.Close(Boolean::New(false));

//    obj->SetInternalField(0, External::New(sem));
    obj->SetAlignedPointerInInternalField(0, sem);
    obj->SetAlignedPointerInInternalField(1, p);

//    obj->SetInternalField(1, Int32::New(id));

    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>pid_act_fnc(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    pid_t id = args[0]->Int32Value();

    errno = 0;
    int er = getpriority(PRIO_PROCESS,id);

    if( er == ESRCH || er == -1) {
        return handle_scope.Close(Boolean::New(false));
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>pid_kill_fnc(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    pid_t id = args[0]->Int32Value();

    int sig = SIGTERM;
    if( args.Length() > 1) sig = SIGKILL;
    kill(id,sig);

    return handle_scope.Close(Boolean::New(true));
}

void js_init()  //--------------------------------------//
{

    Handle<Object> global = context->Global();
    Handle<Object> ob = Object::New();

    //--

    bb_template = (Persistent<v8::ObjectTemplate>)v8::ObjectTemplate::New();
    bb_template->SetInternalFieldCount(3);

    //--

    sql_template = (Persistent<v8::ObjectTemplate>)v8::ObjectTemplate::New();
    sql_template->SetInternalFieldCount(2);

    sql_template->Set(v8::String::NewSymbol("bind"),v8::FunctionTemplate::New(bind)->GetFunction());//id,zn
    sql_template->Set(v8::String::NewSymbol("prep"),v8::FunctionTemplate::New(prepare)->GetFunction());//sql
    sql_template->Set(v8::String::NewSymbol("prepare"),v8::FunctionTemplate::New(prepare)->GetFunction());//sql
    sql_template->Set(v8::String::NewSymbol("exec"),v8::FunctionTemplate::New(exec)->GetFunction());
    sql_template->Set(v8::String::NewSymbol("close"),v8::FunctionTemplate::New(close)->GetFunction());
    sql_template->Set(v8::String::NewSymbol("next"),v8::FunctionTemplate::New(next)->GetFunction());
    sql_template->Set(v8::String::NewSymbol("err"),v8::FunctionTemplate::New(sqlerr)->GetFunction());
    sql_template->Set(v8::String::NewSymbol("rows"),v8::FunctionTemplate::New(rows)->GetFunction());

    //--

    file_template = (Persistent<v8::ObjectTemplate>)v8::ObjectTemplate::New();
    file_template->SetInternalFieldCount(3);

    file_template->Set(v8::String::NewSymbol("wr"),v8::FunctionTemplate::New(fileWr)->GetFunction());//str
    file_template->Set(v8::String::NewSymbol("flush"),v8::FunctionTemplate::New(fileFlush)->GetFunction());//
    file_template->Set(v8::String::NewSymbol("gets"),v8::FunctionTemplate::New(fileGets)->GetFunction());//str
    file_template->Set(v8::String::NewSymbol("gs"),v8::FunctionTemplate::New(fileGets)->GetFunction());//str
    file_template->Set(v8::String::NewSymbol("rd"),v8::FunctionTemplate::New(fileGets)->GetFunction());
    file_template->Set(v8::String::NewSymbol("close"),v8::FunctionTemplate::New(fileClose)->GetFunction());

    //--

    buf_template = (Persistent<v8::ObjectTemplate>)v8::ObjectTemplate::New();
    buf_template->SetInternalFieldCount(3);

    buf_template->Set(v8::String::NewSymbol("slice"),v8::FunctionTemplate::New(buf_slice)->GetFunction());
    buf_template->Set(v8::String::NewSymbol("z"),v8::FunctionTemplate::New(buf_z)->GetFunction());
    buf_template->Set(v8::String::NewSymbol("uz"),v8::FunctionTemplate::New(buf_uz)->GetFunction());
    buf_template->Set(v8::String::NewSymbol("cp"),v8::FunctionTemplate::New(buf_cp)->GetFunction());// copy : buf_from,sz

    buf_template->Set(v8::String::NewSymbol("g8"),v8::FunctionTemplate::New(buf_g8)->GetFunction());// pos?
    buf_template->Set(v8::String::NewSymbol("g16"),v8::FunctionTemplate::New(buf_g16)->GetFunction());
    buf_template->Set(v8::String::NewSymbol("g32"),v8::FunctionTemplate::New(buf_g32)->GetFunction());
    buf_template->Set(v8::String::NewSymbol("gd"),v8::FunctionTemplate::New(buf_gd)->GetFunction());

    buf_template->Set(v8::String::NewSymbol("s8"),v8::FunctionTemplate::New(buf_s8)->GetFunction());// val,pos?
    buf_template->Set(v8::String::NewSymbol("s16"),v8::FunctionTemplate::New(buf_s16)->GetFunction());
    buf_template->Set(v8::String::NewSymbol("s32"),v8::FunctionTemplate::New(buf_s32)->GetFunction());
    buf_template->Set(v8::String::NewSymbol("sd"),v8::FunctionTemplate::New(buf_sd)->GetFunction());

    buf_template->Set(v8::String::NewSymbol("gt"),v8::FunctionTemplate::New(buf_gt)->GetFunction());// pos?,coding_page?
    buf_template->Set(v8::String::NewSymbol("st"),v8::FunctionTemplate::New(buf_st)->GetFunction());// txt,pos?

    buf_template->Set(v8::String::NewSymbol("par"),v8::FunctionTemplate::New(buf_par)->GetFunction());//
    buf_template->Set(v8::String::NewSymbol("b64"),v8::FunctionTemplate::New(buf_b64)->GetFunction());//
    buf_template->Set(v8::String::NewSymbol("clr"),v8::FunctionTemplate::New(buf_clr)->GetFunction());//

    //--

    sock_template = (Persistent<v8::ObjectTemplate>)v8::ObjectTemplate::New();
    sock_template->SetInternalFieldCount(3);

    sock_template->Set(v8::String::NewSymbol("select"),v8::FunctionTemplate::New(scselect)->GetFunction());// txt,pos?
    sock_template->Set(v8::String::NewSymbol("wr"),v8::FunctionTemplate::New(scwr)->GetFunction());// txt,pos?
    sock_template->Set(v8::String::NewSymbol("wr_r"),v8::FunctionTemplate::New(scwr_rest)->GetFunction());// hd,bd?
    sock_template->Set(v8::String::NewSymbol("rd"),v8::FunctionTemplate::New(scrd)->GetFunction());// txt,pos?
    sock_template->Set(v8::String::NewSymbol("rd_r"),v8::FunctionTemplate::New(scrd_rest)->GetFunction());// tm_max?
    sock_template->Set(v8::String::NewSymbol("close"),v8::FunctionTemplate::New(scclose)->GetFunction());// txt,pos?

    sock_template->Set(v8::String::NewSymbol("ww"),v8::FunctionTemplate::New(ww)->GetFunction());// txt ///write web
    sock_template->Set(v8::String::NewSymbol("sb"),v8::FunctionTemplate::New(sendBuf)->GetFunction());// txt ///write web

    //--

    lock_template = (Persistent<v8::ObjectTemplate>)v8::ObjectTemplate::New();
    lock_template->SetInternalFieldCount(2);

    lock_template->Set(v8::String::NewSymbol("ul"),v8::FunctionTemplate::New(lock_close)->GetFunction());//nm?
    lock_template->Set(v8::String::NewSymbol("close"),v8::FunctionTemplate::New(lock_close)->GetFunction());
    lock_template->Set(v8::String::NewSymbol("clr"),v8::FunctionTemplate::New(lock_close)->GetFunction());
    lock_template->Set(v8::String::NewSymbol("del"),v8::FunctionTemplate::New(lock_close)->GetFunction());

    //--

    map_template = (Persistent<v8::ObjectTemplate>)v8::ObjectTemplate::New();
    map_template->SetInternalFieldCount(3);

    map_template->Set(v8::String::NewSymbol("wr"),v8::FunctionTemplate::New(mapWr)->GetFunction());//fn,separator?(~)
    map_template->Set(v8::String::NewSymbol("rd"),v8::FunctionTemplate::New(mapRd)->GetFunction());//separator?(~)

    map_template->Set(v8::String::NewSymbol("clr"),v8::FunctionTemplate::New(mapClr)->GetFunction());
    map_template->Set(v8::String::NewSymbol("find"),v8::FunctionTemplate::New(mapFind)->GetFunction());//key
    map_template->Set(v8::String::NewSymbol("close"),v8::FunctionTemplate::New(mapClose)->GetFunction());
    map_template->Set(v8::String::NewSymbol("del"),v8::FunctionTemplate::New(mapDel)->GetFunction());//key
    map_template->Set(v8::String::NewSymbol("set"),v8::FunctionTemplate::New(mapSet)->GetFunction());//key,val
    map_template->Set(v8::String::NewSymbol("put"),v8::FunctionTemplate::New(mapSet)->GetFunction());//key,val
    map_template->Set(v8::String::NewSymbol("get"),v8::FunctionTemplate::New(mapGet)->GetFunction());//key
    map_template->Set(v8::String::NewSymbol("end"),v8::FunctionTemplate::New(mapEnd)->GetFunction());


    map_template->Set(v8::String::NewSymbol("next"),v8::FunctionTemplate::New(mapNext)->GetFunction());
    map_template->Set(v8::String::NewSymbol("prev"),v8::FunctionTemplate::New(mapPrev)->GetFunction());
    map_template->Set(v8::String::NewSymbol("size"),v8::FunctionTemplate::New(mapSize)->GetFunction());
    map_template->Set(v8::String::NewSymbol("length"),v8::FunctionTemplate::New(mapSize)->GetFunction());

    //--

    proc_template = (Persistent<v8::ObjectTemplate>)v8::ObjectTemplate::New();
    proc_template->SetInternalFieldCount(2);
///*
//    proc_template->Set(v8::String::NewSymbol("rdl"),v8::FunctionTemplate::New(prcrdline)->GetFunction());
    proc_template->Set(v8::String::NewSymbol("wr"),v8::FunctionTemplate::New(prcWr)->GetFunction());//str
    proc_template->Set(v8::String::NewSymbol("rd"),v8::FunctionTemplate::New(prcRd)->GetFunction());
    proc_template->Set(v8::String::NewSymbol("canrd"),v8::FunctionTemplate::New(prcCanRd)->GetFunction());
    proc_template->Set(v8::String::NewSymbol("isRd"),v8::FunctionTemplate::New(prcCanRd)->GetFunction());
    proc_template->Set(v8::String::NewSymbol("ir"),v8::FunctionTemplate::New(prcCanRd)->GetFunction());
    proc_template->Set(v8::String::NewSymbol("isActiv"),v8::FunctionTemplate::New(prcIsActiv)->GetFunction());
    proc_template->Set(v8::String::NewSymbol("ia"),v8::FunctionTemplate::New(prcIsActiv)->GetFunction());
    proc_template->Set(v8::String::NewSymbol("close"),v8::FunctionTemplate::New(prcClose)->GetFunction());

    //-- !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    ob->Set(v8::String::NewSymbol("lg"),v8::FunctionTemplate::New(v8_js_print_lg)->GetFunction());
    ob->Set(v8::String::NewSymbol("er"),v8::FunctionTemplate::New(v8_js_print_lg_err)->GetFunction());
    ob->Set(v8::String::NewSymbol("sleep"),v8::FunctionTemplate::New(hjs_sleep)->GetFunction());// usec?
    ob->Set(v8::String::NewSymbol("sl"),v8::FunctionTemplate::New(hjs_sleep)->GetFunction());// usec?


    ob->Set(v8::String::NewSymbol("mustQuit"),v8::FunctionTemplate::New(mustQuit)->GetFunction());
    ob->Set(v8::String::NewSymbol("mq"),v8::FunctionTemplate::New(mustQuit)->GetFunction());
    ob->Set(v8::String::NewSymbol("strerror"),v8::FunctionTemplate::New(hh_strerror)->GetFunction());// usec?
    ob->Set(v8::String::NewSymbol("se"),v8::FunctionTemplate::New(hh_strerror)->GetFunction());// usec?
    ob->Set(v8::String::NewSymbol("a2i"),v8::FunctionTemplate::New(hjs_A2i)->GetFunction());// usec?
    ob->Set(v8::String::NewSymbol("a2f"),v8::FunctionTemplate::New(hjs_A2f)->GetFunction());// usec?
    ob->Set(v8::String::NewSymbol("gc"),v8::FunctionTemplate::New(idle)->GetFunction());//garbage collection active
    ob->Set(v8::String::NewSymbol("inc"),v8::FunctionTemplate::New(inc)->GetFunction());// file
    ob->Set(v8::String::NewSymbol("sha1"),v8::FunctionTemplate::New(sha1)->GetFunction());
    ob->Set(v8::String::NewSymbol("rm"),v8::FunctionTemplate::New(rm)->GetFunction());// file
    ob->Set(v8::String::NewSymbol("cm"),v8::FunctionTemplate::New(cm)->GetFunction());// chmod
    ob->Set(v8::String::NewSymbol("ren"),v8::FunctionTemplate::New(ren_hh)->GetFunction());// file

    ob->Set(v8::String::NewSymbol("mkdir"),v8::FunctionTemplate::New(hmkdir)->GetFunction());// dir,prava?
    ob->Set(v8::String::NewSymbol("mkd"),v8::FunctionTemplate::New(hmkdir)->GetFunction());// dir,prava?
    ob->Set(v8::String::NewSymbol("dir"),v8::FunctionTemplate::New(dir)->GetFunction());// dir?,filter?("*.cpp *.cxx *.cc")
    ob->Set(v8::String::NewSymbol("curdir"),v8::FunctionTemplate::New(curdir)->GetFunction());// dir?,filter?("*.cpp *.cxx *.cc")

    ob->Set(v8::String::NewSymbol("timer"),v8::FunctionTemplate::New(timer_prc)->GetFunction());// fnc,msec
    ob->Set(v8::String::NewSymbol("close"),v8::FunctionTemplate::New(wclose)->GetFunction());//
    ob->Set(v8::String::NewSymbol("quit"),v8::FunctionTemplate::New(wclose)->GetFunction());//
    ob->Set(v8::String::NewSymbol("exit"),v8::FunctionTemplate::New(wclose)->GetFunction());//

    ob->Set(v8::String::NewSymbol("file"),v8::FunctionTemplate::New(fileHjs)->GetFunction());//file par ==> fopen(file , par)

    ob->Set(v8::String::NewSymbol("buf"),v8::FunctionTemplate::New(buf)->GetFunction());

    ob->Set(v8::String::NewSymbol("listen"),v8::FunctionTemplate::New(listen)->GetFunction());//num_sock
    ob->Set(v8::String::NewSymbol("connect"),v8::FunctionTemplate::New(connect)->GetFunction());//adr

    ob->Set(v8::String::NewSymbol("hash"),v8::FunctionTemplate::New(makeMap)->GetFunction());// Number?
    ob->Set(v8::String::NewSymbol("map"),v8::FunctionTemplate::New(makeMap)->GetFunction());// Number?

    ob->Set(v8::String::NewSymbol("parse"),v8::FunctionTemplate::New(jsParse)->GetFunction());// usec?
    ob->Set(v8::String::NewSymbol("qp"),v8::FunctionTemplate::New(jsParse)->GetFunction());// usec?
    ob->Set(v8::String::NewSymbol("qparse"),v8::FunctionTemplate::New(jsParse)->GetFunction());// usec?

    ob->Set(v8::String::NewSymbol("ld"),v8::FunctionTemplate::New(ld)->GetFunction());// file
    ob->Set(v8::String::NewSymbol("kt"),v8::FunctionTemplate::New(kt)->GetFunction());// usec?


    ob->Set(v8::String::NewSymbol("rd"),v8::FunctionTemplate::New(hread)->GetFunction());// file?,buf?
    ob->Set(v8::String::NewSymbol("wr"),v8::FunctionTemplate::New(wr)->GetFunction());// txt|buf,file?

    ob->Set(v8::String::NewSymbol("proc"),v8::FunctionTemplate::New(proc)->GetFunction());//exec_file par...

    ob->Set(v8::String::NewSymbol("db"),v8::FunctionTemplate::New(db)->GetFunction());//drv,dbname,host,database,user,pass,port?
    ob->Set(v8::String::NewSymbol("dbclose"),v8::FunctionTemplate::New(dbclose)->GetFunction());//dbn?
    ob->Set(v8::String::NewSymbol("sql"),v8::FunctionTemplate::New(sql_func)->GetFunction());//dbn,sql,exec_flg?

    ob->Set(v8::String::NewSymbol("clone"),v8::FunctionTemplate::New(pclone)->GetFunction());//dbn,sql,exec_flg?
    ob->Set(v8::String::NewSymbol("exec"),v8::FunctionTemplate::New(pclone)->GetFunction());//ARGV?,Path?
    ob->Set(v8::String::NewSymbol("ct"),v8::FunctionTemplate::New(ct_fnc)->GetFunction());//name
    ob->Set(v8::String::NewSymbol("pid"),v8::FunctionTemplate::New(pid_fnc)->GetFunction());//dbn,sql,exec_flg?
    ob->Set(v8::String::NewSymbol("pida"),v8::FunctionTemplate::New(pid_act_fnc)->GetFunction());//is pid activ
    ob->Set(v8::String::NewSymbol("kill"),v8::FunctionTemplate::New(pid_kill_fnc)->GetFunction());//is pid activ

    ob->Set(v8::String::NewSymbol("lock"),v8::FunctionTemplate::New(lock_fnc)->GetFunction());//name

    //--

    global->Set(v8::String::NewSymbol("bb"), ob);
    global->Set(v8::String::NewSymbol("ui"), ob);
}
