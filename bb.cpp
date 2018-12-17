#include "bb.h"
#include <iostream>
#include <signal.h>

#ifdef _WIN32
#include <direct.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

#define MSLEEP(x) Sleep(x)

#else

#define MSLEEP(x) usleep(x*1000)

#endif

#include <string.h>

#include "jsmn.h"

#include "mainwindow.h"
using namespace v8;
//using namespace std;
QString rt(const char * s);
extern v8::Persistent<v8::Context> context;
extern MainWindow *w;
extern int run_process;

v8::Handle<v8::Value>out_buf(char *b,int sz);
//v8::Handle<v8::Value>q_parseJS(const char* js);
v8::Handle<v8::Value>q_parseJS(char* s);

#define MAX_RD_DB_SIZE 10000000

int c_scselect( int sock);


Persistent<v8::ObjectTemplate> bb_template;
Persistent<v8::ObjectTemplate> buf_template;
Persistent<v8::ObjectTemplate> sock_template;
Persistent<v8::ObjectTemplate> proc_template;
Persistent<v8::ObjectTemplate> file_template;
Persistent<v8::ObjectTemplate> lock_template;

Persistent<v8::ObjectTemplate> hash_template;
Persistent<v8::ObjectTemplate> map_template;

typedef QHash<QString, QByteArray> HHHashSS_T;
typedef QHash<QString, int64_t> HHHashSN_T;
typedef QHash<int64_t, QByteArray> HHHashNS_T;

typedef QHash<QString, QByteArray>::const_iterator HHHashSS_TI;
typedef QHash<QString, int64_t>::const_iterator HHHashSN_TI;
typedef QHash<int64_t, QByteArray>::const_iterator HHHashNS_TI;

typedef QMap<QString, QByteArray> HHMapSS_T;
typedef QMap<QString, int64_t> HHMapSN_T;
typedef QMap<int64_t, QByteArray> HHMapNS_T;

typedef QMap<QString, QByteArray>::const_iterator HHMapSS_TI;
typedef QMap<QString, int64_t>::const_iterator HHMapSN_TI;
typedef HHMapNS_T::const_iterator HHMapNS_TI;

char * get_file_text_utf8(char * fn,int *fsz);
char * rd(QString s);

#ifndef int64
#define int64 int64_t
#endif

enum{
Buf_ID=20000,Sql_ID,Socket_ID,Proc_ID,File_ID,HashSS_ID,HashSN_ID,HashNS_ID,MapSS_ID,MapSN_ID,MapNS_ID,Lock_ID
};

extern int quit_flg;

extern int argc_;
extern char **argv_;

extern char url_str[100];
char * get_file_text_utf8(char * fn,int *fsz)
{
    char *b = 0;
    FILE *f = fopen(fn,"rb");
    if( f) {
        fseek(f,0,SEEK_END);
        int sz = ftell(f);
        b = (char *)malloc(sz+10);
        fseek(f,0,SEEK_SET);
        fread(b,1,sz,f);
        b[sz] = 0;
        if( fsz) *fsz = sz;

        fclose(f);
    }

    return(b);
}

char * get_resource(char * fn,int *fsz)
{
    if( url_str[0] == 0){
        return get_file_text_utf8(fn,fsz);
    }

//fprintf(stderr,"get_resource 0> \n");fflush(stderr);

    char ser_url[1000];
    strcpy(ser_url,url_str);
    strcat(ser_url,fn);

    UrlRecive *r = get_curl_url_new(ser_url);

    if( r->result != CURLE_OK || !(r->response_code >= 200 && r->response_code < 300)) {

        fprintf(stderr,"get_resource ERROR! res %d %d ser_url: %s fn: %s\n",r->result,r->response_code,ser_url,fn);fflush(stderr);

        delete r;
        return 0;
    }

    if( fsz) *fsz = r->b.size;

    char *rp = r->b.memory;
    r->b.memory = 0;
    delete r;

//    fprintf(stderr,"get_resource %s \n",rp);fflush(stderr);

    return rp;
}

void save_resource(char * fn)
{
    if( url_str[0] == 0 || !fn || !strcmp(fn,"undefined") || !fn[0]) return;

    FILE *f = fopen(fn,"rb");
    if( f){
        fclose(f);
        return;
    }

    int fsz = 0;

//    fprintf(stderr,"save_resource %s \n",fn);fflush(stderr);

    char *r = get_resource(fn,&fsz);
    if( !r) return;

    char dd[1000];
    char *p = fn;

    while( p = strchr(p,'/')){
        int l = p-fn;
        strncpy(dd,fn,l);
        dd[l] = 0;

        int pr = 0755;
        int ret = 0;

        #ifdef _WIN32
            ret = _mkdir(dd);
        #else
            ret = mkdir(dd,pr);
        #endif

        ++p;
    }

    f = fopen(fn,"wb");
    if( f){
        fwrite(r,1,fsz,f);
        fclose(f);
    }
    free(r);
}

char *hh_strrstr(char *string, char *find)
{
    size_t stringlen, findlen;
    char *cp;

    findlen = strlen(find);
    stringlen = strlen(string) - findlen;
    if( stringlen < 0) stringlen = 0;

    if (findlen > stringlen)
        return NULL;

    for (cp = string + stringlen - findlen; cp >= string; cp--)
        if (strncmp(cp, find, findlen) == 0)
            return cp;

    return NULL;
}

static void objectWeakCallback(v8::Persistent<v8::Value> obj, void* parameter)
{
    HandleScope handle_scope;
//fprintf(stderr,"objectWeakCallback ---\n");fflush(stderr);

    v8::Persistent<v8::Object> extObj = v8::Persistent<v8::Object>::Cast(obj);
//fprintf(stderr,"objectWeakCallback --- 0\n");fflush(stderr);

    void * par = (void *)(Handle<External>::Cast(extObj->GetInternalField(0)))->Value();
//fprintf(stderr,"objectWeakCallback --- 1\n");fflush(stderr);
    v8::Handle<v8::External> extObjID = v8::Handle<v8::External>::Cast(extObj->GetInternalField(1));

//fprintf(stderr,"objectWeakCallback --- ID %d\n",extObjID->Int32Value());fflush(stderr);

    switch (extObjID->Int32Value())
    {
        case Buf_ID:
        {
            hBuf *o = (hBuf*) par;
            if( !o->isSlice && o->p && !o->sh.pBuf) {
                free(o->p);
            }
            if( !o->isSlice && o->sh.pBuf) hh_shmem_close(&o->sh);

            delete o;

            break;
        }
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
        case Socket_ID:
        {
            int sc = (int) (Handle<External>::Cast(extObj->GetInternalField(0)))->Int32Value();
            if( sc >= 0) {hh_closesocket(sc);}

            break;
        }
        case Proc_ID:
        {
            hhProcess *o = (hhProcess*) par;
            if( o) {
                o->close();
                delete o;
            }

            break;
        }
    }

    obj.ClearWeak();
//    obj.Dispose();
    obj.Clear();

//    fprintf(stderr,"objectWeakCallback !!!\n");fflush(stderr);
}

static v8::Handle<v8::Value>v8_js_print_lg_err(const Arguments &args)
{
    HandleScope handle_scope;

    QString result = "";
    for (int i = 0; i < args.Length(); ++i){
        String::Utf8Value ascii(args[i]);
        result += rt(*ascii) + " ";
    }

    QByteArray ba;
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    ba = codec->fromUnicode(result);

    fprintf(stderr,"ERR:: %s\n",ba.data());fflush(stderr);

    return handle_scope.Close(Boolean::New(true));
}

static int is_log = 0;

static v8::Handle<v8::Value>v8_js_print_lg(const Arguments &args)
{
    HandleScope handle_scope;
    QString result = "";

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
        String::Utf8Value ascii(args[i]);
        result += rt(*ascii) + " ";
    }

    QByteArray ba;
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    ba = codec->fromUnicode(result);

    fprintf(stderr,"%s\n",ba.data());fflush(stderr);

    return handle_scope.Close(Boolean::New(true));
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

        if( !w->in_t.bin_flg) return handle_scope.Close(Boolean::New(false));

        Local<String> ov = String::New((const char*)w->in_t.b_in);
        w->in_t.bin_flg = 0;

        return handle_scope.Close(ov);
//*/
    }

    return handle_scope.Close(Boolean::New(false));
}

static v8::Handle<v8::Value>inc(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    String::Utf8Value fn(args[0]);

    v8::TryCatch trycatch;

    int fsz=0;
    char *fb = get_resource(*fn,&fsz);
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

static v8::Handle<v8::Value>ld(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    String::Utf8Value fn(args[0]);
    //QPluginLoader


    QLibrary myLib(*fn);
//    QLibrary myLib("");

//    myLib.load();

//    fprintf(stderr,"*************** (%s) %d\n",*fn,myLib.isLoaded());fflush(stderr);

    typedef v8::Handle<v8::Value> (*MyPrototype)(v8::Persistent<v8::Context> ctx, v8::Handle<v8::Value>(*out_buf)(char *b,int sz));
    MyPrototype myFunction = (MyPrototype) myLib.resolve("init");
    if( myFunction) {
//        fprintf(stderr,"FIND!!\n");fflush(stderr);

        return handle_scope.Close(myFunction(context,out_buf));
    }

    return handle_scope.Close(Boolean::New(false));
}


static v8::Handle<v8::Value>ldt(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    String::Utf8Value fn(args[0]);
    //QPluginLoader


    QLibrary myLib(*fn);
//    QLibrary myLib("");

//    myLib.load();

    fprintf(stderr,"ldt *************** (%s) %d\n",*fn,myLib.isLoaded());fflush(stderr);

    typedef int (*MyPrototype)(int ctx);
    MyPrototype myFunction = (MyPrototype) myLib.resolve("init_hh");
    if( myFunction) {
        fprintf(stderr,"FIND!!\n");fflush(stderr);

        myFunction(999);

        return handle_scope.Close(Boolean::New(true));
    }

    return handle_scope.Close(Boolean::New(false));
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

static v8::Handle<v8::Value>to_utf(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    String::Utf8Value drv(args[0]);
fprintf(stderr,"utf :%s\n",(char*)*drv);fflush(stderr);

    QString s = w->tr((char*)*drv);

fprintf(stderr,"utf2 :%s\n",rd(s));fflush(stderr);


Handle<Value> result = Script::Compile(String::New("JSON.stringify('Полная чушь!');"))->Run();
    String::Utf8Value ascii(result);
fprintf(stderr,">>> %s\n",(char*)*ascii);fflush(stderr);


    return handle_scope.Close(Boolean::New(true));
}


static v8::Handle<v8::Value>db(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 6) return handle_scope.Close(Boolean::New(false));

    QSqlDatabase db;// = QSqlDatabase::addDatabase(database);

    String::Utf8Value drv(args[0]);
    String::Utf8Value dbname(args[1]);
    String::Utf8Value host(args[2]);
    String::Utf8Value basenm(args[3]);
    String::Utf8Value user(args[4]);
    String::Utf8Value pass(args[5]);


    if( !strlen(*dbname)) db = QSqlDatabase::addDatabase(*drv);
    else db = QSqlDatabase::addDatabase(*drv,*dbname);

    db.setHostName(*host);
    db.setDatabaseName(*basenm);
    db.setUserName(*user);
    db.setPassword(*pass);

    if( args.Length() > 6) {
        int port = (int)(args[6]->NumberValue());
        db.setPort(port);
    }

    bool ok = db.open();

    if( !ok ) {
        fprintf(stderr,"db.open ERROR::%s>>\n",rd(db.lastError().text()));fflush(stderr);
        return handle_scope.Close(Boolean::New(false));
    } else {
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>dbclose(const Arguments &args)
{
    HandleScope handle_scope;

    char dbn[500] = "";

    if( args.Length() > 0 ) {
        String::Utf8Value nm(args[0]);
        strcpy(dbn,*nm);
    }

    QSqlDatabase db;

    if( !strlen(dbn)) db = QSqlDatabase::database();
    else db = QSqlDatabase::database(dbn);

    if( db.isValid() ) db.close();

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>dbtrans(const Arguments &args)
{
    HandleScope handle_scope;

    char dbn[500] = "";

    if( args.Length() > 0 ) {
        String::Utf8Value nm(args[0]);
        strcpy(dbn,*nm);
    }

    QSqlDatabase db;

    if( !strlen(dbn)) db = QSqlDatabase::database();
    else db = QSqlDatabase::database(dbn);

    if( db.isValid() ) db.transaction();

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>dbcommit(const Arguments &args)
{
    HandleScope handle_scope;

    char dbn[500] = "";

    if( args.Length() > 0 ) {
        String::Utf8Value nm(args[0]);
        strcpy(dbn,*nm);
    }

    QSqlDatabase db;

    if( !strlen(dbn)) db = QSqlDatabase::database();
    else db = QSqlDatabase::database(dbn);

    if( db.isValid() ) db.commit();

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>dbroll(const Arguments &args)
{
    HandleScope handle_scope;

    char dbn[500] = "";

    if( args.Length() > 0 ) {
        String::Utf8Value nm(args[0]);
        strcpy(dbn,*nm);
    }

    QSqlDatabase db;

    if( !strlen(dbn)) db = QSqlDatabase::database();
    else db = QSqlDatabase::database(dbn);

    if( db.isValid() ) db.rollback();

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>bind(const Arguments &args)
{
    HandleScope handle_scope;

    QSqlQuery * ro = (QSqlQuery *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
//    int32_t rid = args.This()->GetInternalField(1)->Int32Value();
//    if( rid != Buf_ID) return Boolean::New(false);

    if( args.Length() < 2) return handle_scope.Close(Boolean::New(false));
    String::Utf8Value idx(args[0]);
    QString ph(*idx);
    ph = ph.simplified();
    if( ph.at(0) != ':') ph = ":" + ph;

//    fprintf(stderr,"ph %s\n",rd(ph));fflush(stderr);

//    if( args[1]->IsString() || args[1]->IsStringObject()) {// string
    if( args[1]->IsString() ) {// string
        String::Utf8Value zn(args[1]);
        ro->bindValue(ph,*zn);

        return handle_scope.Close(Boolean::New(true));
    }
    if( args[1]->IsNumber() ) {//
        double v = (args[1]->NumberValue());
//        v8::Number zn(args[1]);
        ro->bindValue(ph,v);

        return handle_scope.Close(Boolean::New(true));
    }
    if( args[1]->IsBoolean() ) {//
        int v = (int)(args[1]->NumberValue());
//        v8::Number zn(args[1]);
        ro->bindValue(ph,(bool)v);

        return handle_scope.Close(Boolean::New(true));
    }

    // buf

    Local<Object> obj = args[1]->ToObject();
    Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
    void* o = field->Value();
    hBuf * bu = (hBuf * )o;
    int32_t id = obj->GetInternalField(1)->Int32Value();
    if( id != Buf_ID) return handle_scope.Close(Boolean::New(false));

    QByteArray ba = QByteArray::fromRawData((char *)bu->p + bu->beg, bu->end - bu->beg);
    ro->bindValue(ph, ba,QSql::In | QSql::Binary);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>prepare(const Arguments &args)
{
    HandleScope handle_scope;

    QSqlQuery * ro = (QSqlQuery *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
//    int32_t rid = args.This()->GetInternalField(1)->Int32Value();
//    if( rid != Buf_ID) return Boolean::New(false);

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));
    String::Utf8Value sql(args[0]);
//    ro->prepare(*sql);

    return handle_scope.Close(Boolean::New(ro->prepare(*sql)));
}


static v8::Handle<v8::Value>exec(const Arguments &args)
{
    HandleScope handle_scope;

    QSqlQuery * ro = (QSqlQuery *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
    if( !ro->exec()) {
            return handle_scope.Close(Boolean::New(false));
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>sqlerr(const Arguments &args)
{
    HandleScope handle_scope;

    QSqlQuery * ro = (QSqlQuery *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

    return handle_scope.Close(String::New(rd(ro->lastError().text())));
}

static v8::Handle<v8::Value>rows(const Arguments &args)
{
    HandleScope handle_scope;

    QSqlQuery * ro = (QSqlQuery *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

    return handle_scope.Close(Integer::New(ro->numRowsAffected()));
}


static v8::Handle<v8::Value>close(const Arguments &args)
{
    HandleScope handle_scope;

    QSqlQuery * ro = (QSqlQuery *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
    if( ro) delete ro;
    args.This()->SetInternalField(0, Int32::New(0));

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>next(const Arguments &args)
{
    HandleScope handle_scope;

    QSqlQuery * ro = (QSqlQuery *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
    if( !ro) return handle_scope.Close(Boolean::New(false));
//fprintf(stderr,"next 0\n");fflush(stderr);

    if( !ro->next()) {
//        delete ro;
//        args.This()->SetInternalField(0, Int32::New(0));
//        ro->clear();
//          ro->finish();

        return handle_scope.Close(Boolean::New(false));
    }

    //--

    Local<Object> obj = Object::New();

    QSqlRecord rec = ro->record();
    for(int i=0; i < rec.count(); ++i) {

//        fprintf(stderr,"\nn 0\n");fflush(stderr);

        QString fn = rec.fieldName(i);

        char    fnb[500];
        strcpy(fnb,rd(fn));
        QVariant v = rec.value(i);

//        fprintf(stderr,"%s ",rd(fn));fflush(stderr);
//        fprintf(stderr,"%d\n",v.type());fflush(stderr);

        switch(v.type()) {
        case 5:
        case 2:
        case 4:
        case 3:
            obj->Set(v8::String::NewSymbol(fnb),Number::New(v.toLongLong()));
            break;

        case 1:
            obj->Set(v8::String::NewSymbol(fnb),Boolean::New(v.toBool()));
            break;

        case 6:
            obj->Set(v8::String::NewSymbol(fnb),Number::New(v.toDouble()));
            break;

        case 12://buf
            {
            QByteArray ba = v.toByteArray();
            char *p = (char *)malloc(ba.length() + 10);
            memcpy(p,ba.data(),ba.length());
            p[ba.length()] = 0;

            Persistent<Object> ob = Persistent<Object>::New(buf_template->NewInstance());
            hBuf * o = new hBuf();
            o->end = ba.length();
            o->sz = o->end + 1;
            o->p = p;

            ob.MakeWeak(o, objectWeakCallback);

            ob->SetInternalField(0, External::New(o));
            ob->SetInternalField(1, Int32::New(Buf_ID));

            obj->Set(v8::String::NewSymbol(fnb),ob);
//            ob.Dispose();

            }
            break;

        case 10:
        default:

//            fprintf(stderr,"next 10\n");fflush(stderr);
            QString qs = v.toString();
//            fprintf(stderr,"next 10.01 %d\n",qs.length());fflush(stderr);
//            if( qs.length() > 1000000) fprintf(stderr,"next --------------->> 10.01 %d\n",qs.length());fflush(stderr);

            QByteArray ba;
            QTextCodec *codec = QTextCodec::codecForName("UTF-8");
            ba = codec->fromUnicode(qs);

//            return(ba.data());

            char *pp = ba.data();

//            fprintf(stderr,"next 10.1<%lx>\n",pp);fflush(stderr);
//            fprintf(stderr,"next 10.2<%s>\n",pp);fflush(stderr);

            obj->Set(v8::String::NewSymbol(fnb),v8::String::New(pp));

//            fprintf(stderr,"next 10.99\n");fflush(stderr);
            break;
        }
    }

//    fprintf(stderr,"next 99\n");fflush(stderr);

    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>sql(const Arguments &args)
{
    HandleScope handle_scope;

    char dbn[500] = "";

    if( args.Length() > 0 ) {
        String::Utf8Value nm(args[0]);
        strcpy(dbn,*nm);
    }

    QSqlDatabase db;

    if( !strlen(dbn)) db = QSqlDatabase::database();
    else db = QSqlDatabase::database(dbn);

    if( !db.isValid() ) return handle_scope.Close(Boolean::New(false));

    if( args.Length() < 2 ) return handle_scope.Close(Boolean::New(false));

    String::Utf8Value sql(args[1]);

    if( args.Length() > 2 ) {//exec
        QSqlQuery qsql(db);

        if( !qsql.exec(*sql) ) {
            fprintf(stderr,"qsql ERROR:: %s>>\n",rd(qsql.lastError().text()));fflush(stderr);
        }

        return handle_scope.Close(Boolean::New(true));
    }

    QSqlQuery *qsql = new QSqlQuery(db);
    qsql->prepare(*sql);

    int id = Sql_ID;
    Persistent<Object> obj = Persistent<Object>::New(bb_template->NewInstance());

    obj->Set(v8::String::NewSymbol("bind"),v8::FunctionTemplate::New(bind)->GetFunction());//id,zn
    obj->Set(v8::String::NewSymbol("prep"),v8::FunctionTemplate::New(prepare)->GetFunction());//sql
    obj->Set(v8::String::NewSymbol("prepare"),v8::FunctionTemplate::New(prepare)->GetFunction());//sql
    obj->Set(v8::String::NewSymbol("exec"),v8::FunctionTemplate::New(exec)->GetFunction());
    obj->Set(v8::String::NewSymbol("close"),v8::FunctionTemplate::New(close)->GetFunction());
    obj->Set(v8::String::NewSymbol("next"),v8::FunctionTemplate::New(next)->GetFunction());
    obj->Set(v8::String::NewSymbol("err"),v8::FunctionTemplate::New(sqlerr)->GetFunction());
    obj->Set(v8::String::NewSymbol("rows"),v8::FunctionTemplate::New(rows)->GetFunction());

//    obj.MakeWeak(qsql, objectWeakCallback);

    obj->SetInternalField(0, External::New(qsql));
    obj->SetInternalField(1, Int32::New(id));
//    obj.Dispose();

    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>curdir(const Arguments &args)
{
    HandleScope handle_scope;

    QDir dir("./","*");

    Local<String> ov = String::New((const char*)rd(dir.absolutePath()));

    return handle_scope.Close(ov);
}

static v8::Handle<v8::Value>dir(const Arguments &args)
{
    HandleScope handle_scope;

//    if( args.Length() < 1) return Boolean::New(false);

    QDir::Filters filter = QDir::AllDirs | QDir::Files | QDir::Hidden | QDir::NoSymLinks;
    QDir::SortFlags srt = QDir::Name | QDir::DirsFirst;

    char fn[2000] = "./";

    if( args.Length() > 0) {
        String::Utf8Value fnn(args[0]);
        strcpy(fn,*fnn);
    }

    QString nfl = "*";

    if( args.Length() > 1) {
        String::Utf8Value ff(args[1]);
        nfl = *ff;
    }

//    if( args.Length() > 1) filter = (QDir::Filters) args[1]->Int32Value();
//    if( args.Length() > 2) srt = (QDir::SortFlags) args[2]->Int32Value();

//fprintf(stderr,"dir 1  \n");fflush(stderr);

    QDir dir(fn,nfl);

    dir.setFilter(filter);
    dir.setSorting(srt);

//fprintf(stderr,"dir 2  \n");fflush(stderr);

    Local<Array> ar = Array::New();

     QFileInfoList list = dir.entryInfoList();
     for (int i = 0; i < list.size(); ++i) {
        QFileInfo fi = list.at(i);

        Local<Object> obj = Object::New();

        obj->Set(v8::String::NewSymbol("file"),String::New(rd(fi.fileName())));
        obj->Set(v8::String::NewSymbol("filepath"),String::New(rd(fi.filePath())));
        obj->Set(v8::String::NewSymbol("abs"),String::New(rd(fi.absolutePath())));
        obj->Set(v8::String::NewSymbol("path"),String::New(rd(fi.path())));
//        obj->Set(v8::String::NewSymbol("filepath"),String::New(rd(fi.filePath())));
        obj->Set(v8::String::NewSymbol("suffix"),String::New(rd(fi.suffix())));
        obj->Set(v8::String::NewSymbol("size"),Integer::New(fi.size()));

        obj->Set(v8::String::NewSymbol("isDir"),Boolean::New(fi.isDir()));
        obj->Set(v8::String::NewSymbol("isExe"),Boolean::New(fi.isExecutable()));
        obj->Set(v8::String::NewSymbol("isFile"),Boolean::New(fi.isFile()));
        obj->Set(v8::String::NewSymbol("isHid"),Boolean::New(fi.isHidden()));

        Local<Object> tm = Object::New();
        QDateTime dttm(fi.lastModified());

        tm->Set(v8::String::NewSymbol("y"),Int32::New(dttm.date().year()));
        tm->Set(v8::String::NewSymbol("m"),Int32::New(dttm.date().month()));
        tm->Set(v8::String::NewSymbol("d"),Int32::New(dttm.date().day()));

        tm->Set(v8::String::NewSymbol("h"),Int32::New(dttm.time().hour()));
        tm->Set(v8::String::NewSymbol("mi"),Int32::New(dttm.time().minute()));
        tm->Set(v8::String::NewSymbol("s"),Int32::New(dttm.time().second()));

        obj->Set(v8::String::NewSymbol("dttm"),tm);

        ar->Set(v8::Number::New(i),obj);
     }

    return handle_scope.Close(ar);
}

extern int curl_set_timeout;
extern int curl_set_connection_timeout;
extern int curl_set_dns_timeout;
extern int curl_set_low_speed_limit;
extern int curl_set_low_speed_time;
extern char cookiestring[5000];
extern int curl_set_max_red;
extern char curl_set_proxystring[5000];
extern char curl_set_proxytype[5000];

static v8::Handle<v8::Value>curl_set_opt(const Arguments &args)
{
    HandleScope handle_scope;
    int zn = 0;

    curl_set_timeout = 20;
    curl_set_connection_timeout = 10;
    curl_set_dns_timeout = 10;
    curl_set_low_speed_limit = 0;
    curl_set_low_speed_time = 0;
    curl_set_max_red = 6;
    cookiestring[0] = 0;

    curl_set_proxystring[0] = 0;
    curl_set_proxytype[0] = 0;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    Local<Object> obj = args[0]->ToObject();
    Local<String> key;

    if( obj->Has(v8::String::NewSymbol("timeout")) ) {
        zn = obj->Get(v8::String::NewSymbol("timeout"))->Int32Value();
        if( zn) curl_set_timeout = zn;
    }
    if( obj->Has(v8::String::NewSymbol("max_red")) ) {
        zn = obj->Get(v8::String::NewSymbol("max_red"))->Int32Value();
        if( zn) curl_set_max_red = zn;
    }

//fprintf(stderr,"curl_set_timeout %d\n",curl_set_timeout);fflush(stderr);

    if( obj->Has(v8::String::NewSymbol("proxypass")) ) {
        String::Utf8Value proxypass(obj->Get(v8::String::NewSymbol("proxypass")));
        if ((strcmp(*proxypass,"undefined") != 0) && proxypass.length() > 0) {
fprintf(stderr,"-----proxypass %s\n",*proxypass);fflush(stderr);
            strcpy(curl_set_proxystring,*proxypass);
        }
    }

    if( obj->Has(v8::String::NewSymbol("proxytype")) ) {
        String::Utf8Value proxytype(obj->Get(v8::String::NewSymbol("proxytype")));
        if ((strcmp(*proxytype,"undefined") != 0) && proxytype.length() > 0) {
//fprintf(stderr,"proxytype %s\n",*proxytype);fflush(stderr);
            strcpy(curl_set_proxytype,*proxytype);
        }
    }

    if( obj->Has(v8::String::NewSymbol("connection_timeout")) ) {
        zn = obj->Get(v8::String::NewSymbol("connection_timeout"))->Int32Value();
        if( zn) curl_set_connection_timeout = zn;
    }

//fprintf(stderr,"curl_set_connection_timeout %d\n",curl_set_connection_timeout);fflush(stderr);

    if( obj->Has(v8::String::NewSymbol("dns_timeout")) ) {
        zn = obj->Get(v8::String::NewSymbol("dns_timeout"))->Int32Value();
        if( zn) curl_set_dns_timeout = zn;
    }

//fprintf(stderr,"curl_set_dns_timeout %d\n",curl_set_dns_timeout);fflush(stderr);

    if( obj->Has(v8::String::NewSymbol("low_speed_limit")) ) {
        zn = obj->Get(v8::String::NewSymbol("low_speed_limit"))->Int32Value();
        if( zn) curl_set_low_speed_limit = zn;
    }

//fprintf(stderr,"curl_set_low_speed_limit %d\n",curl_set_low_speed_limit);fflush(stderr);

    if( obj->Has(v8::String::NewSymbol("low_speed_time")) ) {
        zn = obj->Get(v8::String::NewSymbol("low_speed_time"))->Int32Value();
        if( zn) curl_set_low_speed_time = zn;
    }

//fprintf(stderr,"curl_set_low_speed_time %d\n",curl_set_low_speed_time);fflush(stderr);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>curl(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    String::Utf8Value url(args[0]);

    if( args.Length() > 1) {
        String::Utf8Value cookie(args[1]);
        strcpy(cookiestring,*cookie);
    } else {
        strcpy(cookiestring,"");
    }

    char *ctx,*hd;
    int sz,hsz;

    ctx = get_curl_url(*url,&sz,&hd,&hsz);

    Local<Object> obj = Object::New();

    int id = Buf_ID;
    Persistent<Object> obj_ctx = Persistent<Object>::New(buf_template->NewInstance());
    hBuf * oc = new hBuf();
    oc->p = ctx;
    oc->end = oc->sz = sz;

    obj_ctx.MakeWeak(oc, objectWeakCallback);
    obj_ctx->SetInternalField(0, External::New(oc));
    obj_ctx->SetInternalField(1, Int32::New(id));

    Persistent<Object> obj_hd = Persistent<Object>::New(buf_template->NewInstance());
    hBuf * oh = new hBuf();
    oh->p = hd;
    oh->end = oh->sz = hsz;

    obj_hd.MakeWeak(oh, objectWeakCallback);
    obj_hd->SetInternalField(0, External::New(oh));
    obj_hd->SetInternalField(1, Int32::New(id));


    obj->Set(v8::String::NewSymbol("ctx"),obj_ctx);
    obj->Set(v8::String::NewSymbol("hd"),obj_hd);

    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>ncurl(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    String::Utf8Value url(args[0]);

    if( args.Length() > 1) {
        String::Utf8Value cookie(args[1]);
        strcpy(cookiestring,*cookie);
    } else {
        strcpy(cookiestring,"");
    }

    UrlRecive *r = get_curl_url_new( *url);

//fprintf(stderr,"ncurl 1\n");fflush(stderr);

    if( r->result != CURLE_OK || !(r->response_code >= 200 && r->response_code < 300)) {
        delete r;
        return handle_scope.Close(Boolean::New(false));
    }

//fprintf(stderr,"ncurl 2\n");fflush(stderr);

    Local<Object> obj = Object::New();

    int id = Buf_ID;
    Persistent<Object> obj_ctx = Persistent<Object>::New(buf_template->NewInstance());
    hBuf * oc = new hBuf();
    oc->p = r->b.memory;
    oc->end = oc->sz = r->b.size;

    obj_ctx.MakeWeak(oc, objectWeakCallback);
    obj_ctx->SetInternalField(0, External::New(oc));
    obj_ctx->SetInternalField(1, Int32::New(id));

    obj->Set(v8::String::NewSymbol("b"),obj_ctx);

    r->b.memory = 0;

//fprintf(stderr,"ncurl 3\n");fflush(stderr);

    char *p = r->h.memory;

//fprintf(stderr,"ncurl 3.1 %ld %s\n------------------\n",p,p);fflush(stderr);

    char *pp = hh_strrstr( p,"\x0D\x0A\x0D\x0A");

//fprintf(stderr,"ncurl 3.5 \n");fflush(stderr);

    if( pp) {
//fprintf(stderr,"ncurl 3.55!! (%s)%ld\n",p,pp);fflush(stderr);
        p = pp + 4;

//fprintf(stderr,"ncurl 3.6!! (%s)%ld %d %d %d\n",p,pp,strlen(pp),strlen(p),pp-p);fflush(stderr);
    }else if( pp = hh_strrstr( p,"\x0A\x0A")){
        p = pp + 2;
    }
    obj->Set(v8::String::NewSymbol("h"),String::New( p));
    obj->Set(v8::String::NewSymbol("hh"),String::New( r->h.memory));

    obj->Set(v8::String::NewSymbol("url"),String::New( r->url.c_str()));
    obj->Set(v8::String::NewSymbol("eurl"),String::New( r->eurl.c_str()));
    obj->Set(v8::String::NewSymbol("ctype"),String::New( r->ctype.c_str()));

//fprintf(stderr,"ncurl 4\n");fflush(stderr);

    delete r;

/*
    char *ctx,*hd;
    int sz,hsz;

    ctx = get_curl_url(*url,&sz,&hd,&hsz);

    Local<Object> obj = Object::New();

    int id = Buf_ID;
    Persistent<Object> obj_ctx = Persistent<Object>::New(buf_template->NewInstance());
    hBuf * oc = new hBuf();
    oc->p = ctx;
    oc->end = oc->sz = sz;

    obj_ctx.MakeWeak(oc, objectWeakCallback);
    obj_ctx->SetInternalField(0, External::New(oc));
    obj_ctx->SetInternalField(1, Int32::New(id));

    Persistent<Object> obj_hd = Persistent<Object>::New(buf_template->NewInstance());
    hBuf * oh = new hBuf();
    oh->p = hd;
    oh->end = oh->sz = hsz;

    obj_hd.MakeWeak(oh, objectWeakCallback);
    obj_hd->SetInternalField(0, External::New(oh));
    obj_hd->SetInternalField(1, Int32::New(id));


    obj->Set(v8::String::NewSymbol("ctx"),obj_ctx);
    obj->Set(v8::String::NewSymbol("hd"),obj_hd);
*/
    return handle_scope.Close(obj);
}


extern CURL *curl_hd;
void hh_curl_set_opt(CURL *curl_handle);

static v8::Handle<v8::Value>vcurl(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    String::Utf8Value url(args[0]);

/*
    if( args.Length() > 1) {
        String::Utf8Value cookie(args[1]);
        strcpy(cookiestring,*cookie);
    } else {
        strcpy(cookiestring,"");
    }
*/

//    UrlRecive *r = get_curl_url_new( *url);

    //--

    CURL *curl_handle;
    CURLcode res;
    int cur_ua = 0;

    UrlRecive *r = new UrlRecive( *url,0);

    if( !curl_hd) {
        curl_hd = curl_handle = curl_easy_init();
    }else{
        curl_handle = curl_hd;
    }

    curl_easy_setopt( curl_handle, CURLOPT_URL, *url);
    hh_curl_set_opt( curl_handle);

    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&r->b);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEHEADER, (void *)&r->h);

    struct curl_slist *headers=NULL; // init to NULL is important
    char *post_s = NULL;
    char *qookie_s = NULL;

    if( args.Length() > 1) {
        Local<Object> o = args[1]->ToObject();

        if( o->Has(v8::String::NewSymbol("q")) ) {
            String::Utf8Value s(o->Get(v8::String::NewSymbol("q")));

            char* qookie_s = (char*)malloc(s.length()+ 5);
            strcpy(qookie_s,*s);

            curl_easy_setopt(curl_handle, CURLOPT_COOKIE, qookie_s);
        }

        if( o->Has(v8::String::NewSymbol("p")) ) {
            String::Utf8Value s(o->Get(v8::String::NewSymbol("p")));

            char* post_s = (char*)malloc(s.length()+ 5);
            strcpy(post_s,*s);

            curl_easy_setopt(curl_handle, CURLOPT_POST, 1);
            curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, post_s);
        }else{
            curl_easy_setopt(curl_handle, CURLOPT_POST, 0L);
        }

        if( o->Has(v8::String::NewSymbol("v")) ) {
            curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
/*
            FILE *fh = fopen("curl_std_err.out", "w+");
            curl_easy_setopt(curl_handle, CURLOPT_STDERR, fh);

            fprintf(stderr,"OK VER 2\n");fflush(stderr);
*/
        }else{
            curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0L);
        }

        if( o->Has(v8::String::NewSymbol("h")) ) {
            Handle<Value> v = o->Get(v8::String::NewSymbol("h"));
            v8::Handle<v8::Array> obj = v8::Handle<v8::Array>::Cast(v);

            int length = obj->Get(v8::String::New("length"))->ToObject()->Uint32Value();

//            struct curl_slist *headers=NULL; // init to NULL is important
//            headers = curl_slist_append(headers, "Accept: application/json");

            for(int i = 0; i < length; i++)
            {
                String::Utf8Value s(obj->Get(i));

//                headers = curl_slist_append(headers, *s);
    //            fprintf(stderr,"pclone %s %d %d\n",*s,s.length(),length);fflush(stderr);

                char* p = (char*)malloc(s.length()+ 5);
                strcpy(p,*s);
                headers = curl_slist_append(headers, p);

//                fprintf(stderr,"headers %s\n",p);fflush(stderr);

//                a[i] = p;
            }

            curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);

        }
    }

    /* get it! */
    res = curl_easy_perform(curl_handle);

    if( headers) curl_slist_free_all(headers);
    if( post_s) free(post_s);
    if( qookie_s) free(qookie_s);

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

    //--

//fprintf(stderr,"vcurl %d %d \n",r->result,r->response_code);fflush(stderr);

    if( r->result != CURLE_OK || !(r->response_code >= 200 && r->response_code < 300)) {
        delete r;
        return handle_scope.Close(Boolean::New(false));
    }

//fprintf(stderr,"vncurl 2\n");fflush(stderr);

    Local<Object> obj = Object::New();

    int id = Buf_ID;
    Persistent<Object> obj_ctx = Persistent<Object>::New(buf_template->NewInstance());
    hBuf * oc = new hBuf();
    oc->p = r->b.memory;
    oc->end = oc->sz = r->b.size;

    obj_ctx.MakeWeak(oc, objectWeakCallback);
    obj_ctx->SetInternalField(0, External::New(oc));
    obj_ctx->SetInternalField(1, Int32::New(id));

    obj->Set(v8::String::NewSymbol("b"),obj_ctx);

    r->b.memory = 0;

//fprintf(stderr,"ncurl 3\n");fflush(stderr);

    char *p = r->h.memory;

//fprintf(stderr,"ncurl 3.1 %ld %s\n------------------\n",p,p);fflush(stderr);

    char *pp = hh_strrstr( p,"\x0D\x0A\x0D\x0A");

//fprintf(stderr,"ncurl 3.5 \n");fflush(stderr);

    if( pp) {
//fprintf(stderr,"ncurl 3.55!! (%s)%ld\n",p,pp);fflush(stderr);
        p = pp + 4;

//fprintf(stderr,"ncurl 3.6!! (%s)%ld %d %d %d\n",p,pp,strlen(pp),strlen(p),pp-p);fflush(stderr);
    }else if( pp = hh_strrstr( p,"\x0A\x0A")){
        p = pp + 2;
    }
    obj->Set(v8::String::NewSymbol("h"),String::New( p));
    obj->Set(v8::String::NewSymbol("hh"),String::New( r->h.memory));

    obj->Set(v8::String::NewSymbol("url"),String::New( r->url.c_str()));
    obj->Set(v8::String::NewSymbol("eurl"),String::New( r->eurl.c_str()));
    obj->Set(v8::String::NewSymbol("ctype"),String::New( r->ctype.c_str()));

//fprintf(stderr,"ncurl 4\n");fflush(stderr);

    delete r;

    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>curlm(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));
    if( !args[0]->IsArray()) return handle_scope.Close(Boolean::New(false));

//    String::Utf8Value url(args[0]);

    if( args.Length() > 1) {
        String::Utf8Value cookie(args[1]);
        strcpy(cookiestring,*cookie);
    } else {
        strcpy(cookiestring,"");
    }

    //String::Utf8Value url(args[0]);
    Handle<Value> v = args[0];
    v8::Handle<v8::Array> obj = v8::Handle<v8::Array>::Cast(v);

    int length = obj->Get(v8::String::New("length"))->ToObject()->Uint32Value();

    std::vector<std::string> a;

    for(int i = 0; i < length; i++)
    {
        String::Utf8Value s(obj->Get(i));
//        fprintf(stderr,"curlm %s\n",*s);fflush(stderr);

        a.push_back( *s);
    }

    int mth = (length > 100)?100:length;

    if( args.Length() > 2) {
        mth = args[1]->Int32Value();
    }

    std::vector<UrlRecive*> *rr = hh_curl_mul(a, mth);

    Local<Array> oa = Array::New();

    int outc = 0;
    for( int n =0; n < rr->size(); ++n ) {

        UrlRecive *r = rr->at(n);

//        std::cout << n <<" "<< r->url << " res: " << r->result << " response_code " << r->response_code <<" -OOUT- \n";

        if( r->result != CURLE_OK || !(r->response_code >= 200 && r->response_code < 300)) {
//            std::cout << n <<" "<< r->url << " res: " << r->result << " response_code " << r->response_code <<" -OOUT- \n";
            delete r;
            continue;
        }

        Local<Object> obj = Object::New();

        int id = Buf_ID;
        Persistent<Object> obj_ctx = Persistent<Object>::New(buf_template->NewInstance());
        hBuf * oc = new hBuf();
        oc->p = r->b.memory;
        oc->end = oc->sz = r->b.size;

        obj_ctx.MakeWeak(oc, objectWeakCallback);
        obj_ctx->SetInternalField(0, External::New(oc));
        obj_ctx->SetInternalField(1, Int32::New(id));

        obj->Set(v8::String::NewSymbol("b"),obj_ctx);

        r->b.memory = 0;

        char *p = r->h.memory;
        char *pp = hh_strrstr( p,"\x0D\x0A\x0D\x0A");
        if( pp) {
            p = pp + 4;
        }else if( pp = hh_strrstr( p,"\x0A\x0A")){
            p = pp + 2;
        }
        obj->Set(v8::String::NewSymbol("h"),String::New( p));
        obj->Set(v8::String::NewSymbol("hh"),String::New( r->h.memory));

        obj->Set(v8::String::NewSymbol("url"),String::New( r->url.c_str()));
        obj->Set(v8::String::NewSymbol("eurl"),String::New( r->eurl.c_str()));
        obj->Set(v8::String::NewSymbol("ctype"),String::New( r->ctype.c_str()));
        obj->Set(v8::String::NewSymbol("n"),Integer::New(r->cnt));

        delete r;

        oa->Set(v8::Number::New(outc),obj);

        ++outc;
    }

    delete rr;

    return handle_scope.Close(oa);
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
//    pos = "TEST";

    return handle_scope.Close(String::New(pos));
}

static v8::Handle<v8::Value>buf_b64(const Arguments &args)
{
    HandleScope handle_scope;

    hBuf * ro = (hBuf *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

    int beg=0;
    if( args.Length() > 0) beg = args[0]->Int32Value();

    char * pos = ro->p + beg;
    QByteArray ba(pos,ro->sz);
    QByteArray ba2 = ba.toBase64();

    return handle_scope.Close(String::New(ba2.data()));
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
        int64 *i64 = (int64 *)ro->p;
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
    int64 *i64 = (int64 *)o->p;
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

static v8::Handle<v8::Value>scclose(const Arguments &args)
{
    HandleScope handle_scope;

    int sc = (int)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Int32Value();
    if( sc >= 0) hh_closesocket(sc);

    args.This()->SetInternalField(0, Int32::New(-1));
    return handle_scope.Close(Boolean::New(true));
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
/*
        String::Utf8Value bd(args[1]);

//        char b[100];
//        sprintf(b,"\nContent-Length: %d\n\n",bd.length());

        char *bb = (char *) malloc(hd.length() + bd.length() + 200);
        sprintf(bb,"%s\nContent-Length: %d\n\n%s",*hd,bd.length(),*bd);
        send_buf(sc,bb,0);
        free( bb);

//        send_buf(sc,b,0);
//        send_buf(sc,*bd,bd.length());
*/
        if( args[1]->IsString() || args[1]->IsNumber()) {
            String::Utf8Value bd(args[1]);

    //        char b[100];
    //        sprintf(b,"\nContent-Length: %d\n\n",bd.length());

            char *bb = (char *) malloc(hd.length() + bd.length() + 200);
            sprintf(bb,"%s\nContent-Length: %d\n\n%s",*hd,bd.length(),*bd);
            send_buf(sc,bb,0);
//            fprintf(stderr,"bb<%s>\n",bb);fflush(stderr);

            free( bb);

    //        send_buf(sc,b,0);
    //        send_buf(sc,*bd,bd.length());
        }else{

            Local<Object> obj = args[1]->ToObject();
            Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
            hBuf* o = (hBuf*)field->Value();

            char *bb = (char *) malloc(hd.length() + o->sz + 200);
            sprintf(bb,"%s\nContent-Length: %d\n\n",*hd,o->sz);//,o->p);
            int bb_l = strlen(bb);
            int f_sz = bb_l + o->sz;
            memcpy(bb + bb_l, o->p, o->sz) ;
            send_buf(sc,bb,f_sz);
            free( bb);

            FILE *f = fopen("t1.jpg","wb");
            fwrite(o->p,1,o->sz,f);
            fclose(f);
        }



    } else {
        char *bb = (char *) malloc(hd.length() + 200);
        sprintf(bb,"%s\n\n",*hd);
        send_buf(sc,bb,0);
        free( bb);

//        send_buf(sc,"\n\n",0);
    }

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

static v8::Handle<v8::Value>rest(const Arguments &args)// url_ser,cmd,url,body,szm,add_s
{
    HandleScope handle_scope;
    int sc=0;

    if( args.Length() < 4) return handle_scope.Close(Boolean::New(false));

    String::Utf8Value adr(args[0]);
    String::Utf8Value cmd(args[1]);
    String::Utf8Value url(args[2]);
    QString adr_s(*adr);
    QString cmd_s(*cmd);
    QString url_s(*url);
//    QString body_s;

//    String::Utf8Value body(args.Length() > 3?args[3]:"");
    String::Utf8Value body(args[3]);
/*
    if( args.Length() > 3) {
        String::Utf8Value body(args[3]);
        body_s = rt(*body);
    }
*/
    QString add_s;

    if( args.Length() > 5) {
        String::Utf8Value add(args[5]);
        add_s = rt(*add);
    }


    int szm = 100000;

    if( args.Length() > 4) {
        szm = args[4]->Int32Value();
    }

    sc = hh_connect ((char*)*adr,0);
    if( sc < 0 ) return handle_scope.Close(Boolean::New(false));

    QUrl u(url_s);
    url_s = rt(u.toEncoded().data());
    QString txt = cmd_s + " " +  url_s + " HTTP/1.1\n" + add_s + "Accept: application/json\nContent-Type: application/json\nUser-Agent:Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.11 (KHTML, like Gecko) Chrome/23.0.1271.91 Safari/537.11\n"+ QString("Content-Length: %1").arg(body.length()) +"\015\012\015\012";

fprintf(stderr,"\nsnd: (%s%s)\n",rd(txt),*body);fflush(stderr);

    int r = send_buf( sc, rd(txt), 0);
    if( body.length() > 0) r = send_buf( sc, *body, body.length());

//fprintf(stderr,"body: %d[%s]",body.length(),*body);fflush(stderr);

    // recive

    char *blk = (char *) malloc(szm + 10);
    if(!blk) {
        hh_closesocket(sc);
        return handle_scope.Close(Boolean::New(false));
    }

    int tot = 0;
    char *p = (char*)blk;

    do {
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

        p += ret;
        tot += ret;

        if( (szm-tot) < (szm/10)) {
            int new_szm = szm *2;
            blk = (char*)realloc(blk,new_szm+10);
            if(!blk) {
                hh_closesocket(sc);
                return handle_scope.Close(Boolean::New(false));
            }
            p = &blk[tot];

            szm = new_szm;
        }

    } while(1);

    hh_closesocket(sc);

    blk[tot] = 0;

//    fprintf(stderr,"\nret: (%s)\n",blk);fflush(stderr);

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


//    Local<String> h = String::New(hp);
//    Local<String> b = String::New(bp);


    Local<Object> obj = Object::New();

    obj->Set(v8::String::NewSymbol("h"),String::New(hp));
    obj->Set(v8::String::NewSymbol("b"),String::New(bp));

//    Local<String> str = String::New(blk);
    free(blk);

    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>restb(const Arguments &args)// url_ser,hd,bd,tm_max
{
    HandleScope handle_scope;
    int sc=0;
    int tm_max = 300;

    if( args.Length() < 3) return handle_scope.Close(Boolean::New(false));
    if( args.Length() > 3) {
        tm_max = args[3]->Int32Value();
    }

    String::Utf8Value adr(args[0]);
    String::Utf8Value head(args[1]);

    String::Utf8Value bd(args[2]);

    char *m = (char *)malloc( strlen(*head) + strlen(*bd) + 200);
    sprintf(m,*head,strlen(*bd),*bd);

//    QString hd_s(*head);

    int szm = 100000;

    sc = hh_connect ((char*)*adr,0);
    if( sc < 0 ) {
        free(m);
        return handle_scope.Close(Boolean::New(false));
    }

//    QString txt = hd_s;

fprintf(stderr,"\n\n--------------------m:\n<%s>\n",m);fflush(stderr);

//    int r = send_buf( sc, m, 0);
    int r = send_buf( sc, m, strlen(m));
    if( r){
        fprintf(stderr,"ERR: send_buf-> r=%d\n",r);fflush(stderr);
    }
    free(m);
    fprintf(stderr,"sended...\n");fflush(stderr);

//    if( body.length() > 0) r = send_buf( sc, *body, body.length());

//fprintf(stderr,"body: %d[%s]",body.length(),*body);fflush(stderr);

    // recive

//fprintf(stderr,"\nrec %d:\n",r);fflush(stderr);

    char *blk = (char *) malloc(szm + 10);
    if(!blk) {
        hh_closesocket(sc);
        return handle_scope.Close(Boolean::New(false));
    }

    int tot = 0;
    char *p = (char*)blk;

    time_t ltm = time(0);
//    time_t ctm = time(0);

    do {
        if( !c_scselect(sc)) {
            if( ( time(0) - ltm) > tm_max) {
                fprintf(stderr,"\nTIMER_SOCK_ERROR %d sec!!\n",tm_max);fflush(stderr);

                blk[tot] = 0;

                fprintf(stderr,"\n<%s>%d %d\n",blk,strlen(blk),tot);fflush(stderr);

                break;
            }

            MSLEEP(100);
            continue;
        }
/*
        if( ( time(0) - ltm) > tm_max) {
            fprintf(stderr,"\nTIMER_SOCK_ERROR %d sec!\n",tm_max);fflush(stderr);

            break;
        }
*/

        ltm = time(0);
        int ret = recv (sc,p,szm-tot,0);

        if(ret == 0 ) { //closed
          break;
        }
        if(ret == -1 ) {
          if (errno!=EINTR){
              break;
          } else {
              fprintf(stderr,"\nERR:: errno!=EINTR ret == -1 errno: %d!\n",errno);fflush(stderr);
            continue;
          }
        }

        if( ret > 0) {
            fprintf(stderr,"\n*--> ");fflush(stderr);
            for(int i=0; i < ret; ++i){
                fprintf(stderr,"%c",p[i]);fflush(stderr);
            }
            fprintf(stderr,"<--\n");fflush(stderr);
        }

        p += ret;
        tot += ret;

        if( (szm-tot) < (szm/10)) {
            int new_szm = szm *2;

            blk = (char*)realloc(blk,new_szm+10);

            if(!blk) {
                hh_closesocket(sc);
                return handle_scope.Close(Boolean::New(false));
            }
            p = &blk[tot];

            szm = new_szm;
        }

    } while(1);

    hh_closesocket(sc);

    blk[tot] = 0;

//    fprintf(stderr,"\nret: (%s)\n",blk);fflush(stderr);

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


//    Local<String> h = String::New(hp);
//    Local<String> b = String::New(bp);


    Local<Object> obj = Object::New();

    obj->Set(v8::String::NewSymbol("h"),String::New(hp));
    obj->Set(v8::String::NewSymbol("b"),String::New(bp));

//    Local<String> str = String::New(blk);
    free(blk);

    return handle_scope.Close(obj);
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

//fprintf(stderr,"scrd_rest 10\n");fflush(stderr);

    do {
        if( !c_scselect(sc)) {
            if( ( time(0) - ltm) > tm_max) {
                break;
            }

            MSLEEP(100);
            continue;
        }

        ltm = time(0);

//fprintf(stderr,"scrd_rest 10.1 \n");fflush(stderr);

        int ret = recv (sc,p,szm-tot,0);

//fprintf(stderr,"scrd_rest 11 ret %d errno %d \n",ret,errno);fflush(stderr);

        if(ret == 0 ) { //closed
          break;
        }
        if(ret == -1 ) {
          if (errno != EINTR){
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
        *p = 0;
        tot += ret;

//fprintf(stderr,"%d : %s\n",ret, blk);fflush(stderr);

        if( (szm - tot) < (szm / 10)) {
            int new_szm = szm *2;
            blk = (char*)realloc(blk,new_szm+10);
/*
            char *blk2 = (char*)malloc(new_szm);
            memcpy(blk2,blk,tot);
            free(blk);
            blk = blk2;
*/
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
            bpn = pp - blk;

//            fprintf(stderr,"**** 4 %d\n",pp);fflush(stderr);

        }else if( bpn == 0 && (pp = strstr(blk,"\012\012"))) {
            *pp++ = 0;
            *pp++ = 0;
            bp = pp;
            bpn = pp - blk;

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

//fprintf(stderr,"bd_sz %d\n",bd_sz);fflush(stderr);

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

//fprintf(stderr,"rcvH<%s>LL %d\n",blk,strlen(blk));fflush(stderr);
//fprintf(stderr,"rcvB<%s>LL %d tot:%d\n",bp,strlen(bp),tot);fflush(stderr);

    free(blk);

    //---

    return handle_scope.Close(str);
}

int c_scselect( int sock)
{

    fd_set				scArr;
    timeval				tmW;

    tmW.tv_sec = 0;
    tmW.tv_usec = 2;

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

    if( r > 0 ) {
        if( !listen_flg) return handle_scope.Close(Boolean::New(true)); // connect

        // listen

        int					slave;
        struct sockaddr_in	client;
        socklen_t			clilen;

        clilen=sizeof(client);
        slave=::accept(sock,(struct sockaddr *)&client,&clilen);
        if( slave < 0 ) return handle_scope.Close(Boolean::New(false));

//        return Boolean::New(true);

        int id = Socket_ID;
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

static v8::Handle<v8::Value>proc(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() == 0) return handle_scope.Close(Boolean::New(false));
    String::Utf8Value exe(args[0]);


    int id = Proc_ID;
    Local<Object> obj = Local<Object>::New(proc_template->NewInstance());
    hhProcess * o = new hhProcess();
    o->start(*exe);

    run_process = 1;

//    obj.MakeWeak(o, objectWeakCallback);

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
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

static v8::Handle<v8::Value>hashSet(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 2) return handle_scope.Close(Boolean::New(false));

    int id = (Handle<External>::Cast(args.This()->GetInternalField(1)))->Int32Value();

    if( id == HashSS_ID) {
        String::Utf8Value   key(args[0]);
        String::Utf8Value   val(args[1]);

        HHHashSS_T *o = (HHHashSS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        o->insert(*key,*val);

    } else if( id == HashSN_ID){
        String::Utf8Value   key(args[0]);
        int64_t val   = args[1]->IntegerValue();

        HHHashSN_T *o = (HHHashSN_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        o->insert(*key,val);
    } else if( id == HashNS_ID) {
        String::Utf8Value   val(args[1]);
        int64_t key   = args[0]->IntegerValue();

        HHHashNS_T *o = (HHHashNS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        o->insert(key,*val);
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>hashDel(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    int id = (Handle<External>::Cast(args.This()->GetInternalField(1)))->Int32Value();

    if( id == HashSS_ID) {
        String::Utf8Value   key(args[0]);

        HHHashSS_T *o = (HHHashSS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        o->remove(*key);

    } else if( id == HashSN_ID){
        String::Utf8Value   key(args[0]);

        HHHashSN_T *o = (HHHashSN_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        o->remove(*key);

    } else if( id == HashNS_ID) {
        int64_t key   = args[0]->IntegerValue();

        HHHashNS_T *o = (HHHashNS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        o->remove(key);
    }

    return handle_scope.Close(Boolean::New(true));
}

//--

static v8::Handle<v8::Value>dumpJS(const char *js, jsmntok_t *t, size_t count, int indent,int *jj)
{
    HandleScope handle_scope;
    char b[1000];

//    v8::Handle<v8::Value> o = Boolean::New(false);
    v8::Handle<v8::Value> o = (v8::Handle<v8::Value>)0;

    int i, j, k;
    if (count <= 0) {
        return handle_scope.Close(o);
    }

    if (t->type == JSMN_PRIMITIVE) {
//        printf("%.*s", t->end - t->start, js+t->start);
        *jj += 1;
        char c = js[t->start];
        if( c == 'f' || c == 'F') {
            return handle_scope.Close(Boolean::New(false) );
        }
        if( c == 't' || c == 'T') {
            return handle_scope.Close(Boolean::New(true) );
        }
        if( c == 'n' || c == 'N') {
            return handle_scope.Close( v8::Null() );
        }

        if( (t->end - t->start) >= sizeof(b)) return handle_scope.Close( v8::Null());

        memset(b,0,sizeof(b));
        memcpy(b,js+t->start,t->end - t->start);
        return handle_scope.Close( Number::New( atof( b) ));
    } else if (t->type == JSMN_STRING) {
//        printf("(%.*s)", t->end - t->start, js+t->start);
        *jj += 1;
        return handle_scope.Close(String::New( js+t->start, t->end - t->start) );
    } else if (t->type == JSMN_OBJECT) {
        Local<Object> o = Object::New();

        j = 0;
        for (i = 0; i < t->size; i++) {
//            for (k = 0; k < indent; k++) printf(" O ");
//			j +=

            jsmntok_t *tt = t+1+j;
//            dumpJS(js, t+1+j, count-j, indent+1,&j);
            if( (count-j) <= 0) break;

            if (tt->type != JSMN_STRING || (tt->end - tt->start) >= sizeof(b)) {
                ++j;
                continue;
            }

            memset(b,0,sizeof(b));
            memcpy(b,js + tt->start,tt->end - tt->start);

            ++j;
            v8::Handle<v8::Value> e = dumpJS(js, t+1+j, count-j, indent+1,&j);
            if( e == ( v8::Handle<v8::Value> )0) continue;
            o->Set(v8::String::NewSymbol(b),e);

//            printf("\n");
        }
        *jj += j+1;
        return handle_scope.Close(o);
    } else if (t->type == JSMN_ARRAY) {
        Local<Array> o = Array::New();

        j = 0;
        for (i = 0; i < t->size; i++) {
            v8::Handle<v8::Value> e = dumpJS(js, t+1+j, count-j, indent+1,&j);
            if( e == ( v8::Handle<v8::Value> )0) continue;

            o->Set(v8::Number::New(i),e);
        }
        *jj += j + 1;
        return handle_scope.Close(o);
    }

    return handle_scope.Close(o);
}

static inline void *realloc_it(void *ptrmem, size_t size) {
    void *p = realloc(ptrmem, size);
    if (!p)  {
        free (ptrmem);
        fprintf(stderr, "realloc(): errno=%d\n", errno);
    }
    return p;
}

static v8::Handle<v8::Value>parseJS(const char* js,int jslen)
{
    HandleScope handle_scope;

    jsmn_parser p;
    jsmntok_t *tok;
    size_t tokcount = 10000;
    int r;

    v8::Handle<v8::Value> o = Boolean::New(false);

    jsmn_init( &p);

    /* Allocate some tokens as a start */
    tok = (jsmntok_t *)malloc(sizeof(*tok) * tokcount);
    if (tok == NULL) {
        fprintf(stderr, "malloc(): errno=%d\n", errno);
        return handle_scope.Close(Boolean::New(false));
    }

again:

    r = jsmn_parse(&p, js, jslen, tok, tokcount);
    if (r < 0) {
        if (r == JSMN_ERROR_NOMEM) {
            tokcount = tokcount * 2;
            tok = (jsmntok_t *)realloc_it(tok, sizeof(*tok) * tokcount);
            if (tok == NULL) {
                goto out;
            }
            goto again;
        }
    } else {
        int retd = 0;
        o = dumpJS(js, tok, p.toknext, 0, &retd);
        if( o == ( v8::Handle<v8::Value> )0){
            o = Boolean::New(false);
        }
    }

out:

    if( tok) free(tok);

    return handle_scope.Close(o);
}

//--

static v8::Handle<v8::Value>hashGet(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    int id = (Handle<External>::Cast(args.This()->GetInternalField(1)))->Int32Value();

    if( id == HashSS_ID) {
        String::Utf8Value   key(args[0]);

        HHHashSS_T *o = (HHHashSS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        QByteArray val = o->value(*key);

        if( val.length() == 0) return handle_scope.Close(Boolean::New(false));

        if( args.Length() > 1){
//            return handle_scope.Close( parseJS(val.data(),val.length()));
            return handle_scope.Close( parseJS(val.data()));
        }else {
            Local<String> ov = String::New((const char*)val.data());
            return handle_scope.Close(ov);
        }

    } else if( id == HashSN_ID){
        String::Utf8Value   key(args[0]);

        HHHashSN_T *o = (HHHashSN_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        int64_t val = o->value(*key);

        return handle_scope.Close(v8::Number::New(val));

    } else if( id == HashNS_ID) {
        int64_t key   = args[0]->IntegerValue();

        HHHashNS_T *o = (HHHashNS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        QByteArray val = o->value(key);
        if( val.length() == 0) return handle_scope.Close(Boolean::New(false));

        if( args.Length() > 1){
//            return handle_scope.Close( parseJS(val.data(),val.length()));
            return handle_scope.Close( parseJS(val.data()));
        }else {
            Local<String> ov = String::New((const char*)val.data());
            return handle_scope.Close(ov);
        }
    }

    return handle_scope.Close(Boolean::New(false));
}

static v8::Handle<v8::Value>hashSize(const Arguments &args)
{
    HandleScope handle_scope;

    int id = (Handle<External>::Cast(args.This()->GetInternalField(1)))->Int32Value();

    if( id == HashSS_ID) {

        HHHashSS_T *o = (HHHashSS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        int sz = o->size();

        return handle_scope.Close(Integer::New(sz));

    } else if( id == HashSN_ID){
        HHHashSN_T *o = (HHHashSN_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        int sz = o->size();

        return handle_scope.Close(Integer::New(sz));

    } else if( id == HashNS_ID) {
        HHHashNS_T *o = (HHHashNS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        int sz = o->size();

        return handle_scope.Close(Integer::New(sz));
    }

    return handle_scope.Close(Boolean::New(false));
}


static v8::Handle<v8::Value>hashWr(const Arguments &args)
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
    if( !f) return handle_scope.Close(Boolean::New(false));

    int id = (Handle<External>::Cast(args.This()->GetInternalField(1)))->Int32Value();

    if( id == HashSS_ID) {
        HHHashSS_T *o = (HHHashSS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        HHHashSS_TI oi = o->constBegin();
        while( oi != o->constEnd() ) {
            fprintf(f,"%s%c%s\n",rd(oi.key()),sep,oi.value().data());
            ++oi;
        }

    } else if( id == HashSN_ID){
        HHHashSN_T *o = (HHHashSN_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        HHHashSN_TI oi = o->constBegin();
        while( oi != o->constEnd() ) {
            fprintf(f,"%s%c%ld\n",rd(oi.key()),sep,oi.value());
            ++oi;
        }

    } else if( id == HashNS_ID) {
        HHHashNS_T *o = (HHHashNS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        HHHashNS_TI oi = o->constBegin();
        while( oi != o->constEnd() ) {
            fprintf(f,"%ld%c%s\n",oi.key(),sep,oi.value().data());
            ++oi;
        }
    }

    fclose(f);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>hashRd(const Arguments &args)
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

    if( id == HashSS_ID) {
        HHHashSS_T *o = (HHHashSS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        while( fgets(b,MAX_RD_DB_SIZE,f)) {
            int sl = strlen(b);

            if( b[sl - 1] == 0xd || b[sl - 1] == 0xa) b[sl - 1] = 0;
            if( b[sl - 2] == 0xd || b[sl - 2] == 0xa) b[sl - 2] = 0;
            char *p = strchr(b,sep);
            if( !p) continue;

            *p = 0;++p;
            o->insert(b,p);
        }

    } else if( id == HashSN_ID){
        HHHashSN_T *o = (HHHashSN_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        while( fgets(b,MAX_RD_DB_SIZE,f)) {
            int sl = strlen(b);

            if( b[sl - 1] == 0xd || b[sl - 1] == 0xa) b[sl - 1] = 0;
            if( b[sl - 2] == 0xd || b[sl - 2] == 0xa) b[sl - 2] = 0;
            char *p = strchr(b,sep);
            if( !p) continue;

            *p = 0;++p;
            o->insert(b,atoll(p));
        }

    } else if( id == HashNS_ID) {
        HHHashNS_T *o = (HHHashNS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        while( fgets(b,MAX_RD_DB_SIZE,f)) {
            int sl = strlen(b);

            if( b[sl - 1] == 0xd || b[sl - 1] == 0xa) b[sl - 1] = 0;
            if( b[sl - 2] == 0xd || b[sl - 2] == 0xa) b[sl - 2] = 0;
            char *p = strchr(b,sep);
            if( !p) continue;

            *p = 0;++p;
            o->insert(atoll(p),p);
        }
    }

    free(b);

    fclose(f);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>hashClr(const Arguments &args)
{
    HandleScope handle_scope;

    int id = (Handle<External>::Cast(args.This()->GetInternalField(1)))->Int32Value();

    if( id == HashSS_ID) {
        HHHashSS_T *o = (HHHashSS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        o->clear();
//        delete o;
    } else if( id == HashSN_ID){
        HHHashSN_T *o = (HHHashSN_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        o->clear();
//        delete o;
    } else if( id == HashNS_ID) {
        HHHashNS_T *o = (HHHashNS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        o->clear();
//        delete o;
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>hashClose(const Arguments &args)
{
    HandleScope handle_scope;

    int id = (Handle<External>::Cast(args.This()->GetInternalField(1)))->Int32Value();

    if( id == HashSS_ID) {
        HHHashSS_T *o = (HHHashSS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        o->clear();
        delete o;
    } else if( id == HashSN_ID){
        HHHashSN_T *o = (HHHashSN_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        o->clear();
        delete o;
    } else if( id == HashNS_ID) {
        HHHashNS_T *o = (HHHashNS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        o->clear();
        delete o;
    }

    return handle_scope.Close(Boolean::New(true));
}


static v8::Handle<v8::Value>makeHash(const Arguments &args)
{
    HandleScope handle_scope;

    int id = HashSS_ID;
/*
    if( args.Length() > 0) {
        id = HashSS_ID + args[0]->Int32Value();
    }
*/
    if( args.Length() > 0) {
        if( args[1]->IsBoolean()) {
            id = HashSN_ID;
        }else if( args[1]->IsNumber()){
            id = HashSS_ID + args[0]->Int32Value();
        }else if( args[1]->IsString()){
            String::Utf8Value nm(args[1]);
            if( !strcmp(*nm,"SS")) {
                id = HashSS_ID;
            }else if( !strcmp(*nm,"SN")) {
                id = HashSN_ID;
            }else if( !strcmp(*nm,"NS")) {
                id = HashNS_ID;
            }
        }
    }


//    Persistent<Object> obj = Persistent<Object>::New(hash_template->NewInstance());
    Local<Object> obj = Local<Object>::New(hash_template->NewInstance());

    if( id == HashSS_ID) {
        HHHashSS_T *o = new HHHashSS_T();
        obj->SetInternalField(0, External::New(o));
    } else if( id == HashSN_ID){
        HHHashSN_T *o = new HHHashSN_T();
        obj->SetInternalField(0, External::New(o));
    } else if( id == HashNS_ID) {
        HHHashNS_T *o = new HHHashNS_T();
        obj->SetInternalField(0, External::New(o));
    }

    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
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
        o->insert(*key,*val);

    } else if( id == MapSN_ID){
        String::Utf8Value   key(args[0]);
        int64_t val   = args[1]->IntegerValue();

        HHMapSN_T *o = (HHMapSN_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        o->insert(*key,val);
    } else if( id == MapNS_ID){
        String::Utf8Value   val(args[1]);
        int64_t key   = args[0]->IntegerValue();

        HHMapNS_T *o = (HHMapNS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        o->insert(key,*val);
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
        QByteArray val = o->value(*key);
        if( val.length() == 0) return handle_scope.Close(Boolean::New(false));

        if( args.Length() > 1){
//            return handle_scope.Close( parseJS(val.data(),val.length()));
            return handle_scope.Close( parseJS(val.data()));
        } else {
            Local<String> ov = String::New((const char*)val.data());
            return handle_scope.Close(ov);
        }
//        Local<String> ov = String::New((const char*)val.data());
//        return handle_scope.Close(ov);

    } else if( id == MapSN_ID){
        String::Utf8Value   key(args[0]);

        HHMapSN_T *o = (HHMapSN_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        int64_t val = o->value(*key);

        return handle_scope.Close(v8::Number::New(val));

    } else if( id == MapNS_ID){
        int64_t key   = args[0]->IntegerValue();

        HHMapNS_T *o = (HHMapNS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        QByteArray val = (QByteArray)o->value(key);
        if( val.length() == 0) return handle_scope.Close(Boolean::New(false));

        if( args.Length() > 1){
//            return handle_scope.Close( parseJS(val.data(),val.length()));
            return handle_scope.Close( parseJS(val.data()));
        } else {
            Local<String> ov = String::New((const char*)val.data());
            return handle_scope.Close(ov);
        }
    }
    return handle_scope.Close(Boolean::New(false));
}

static v8::Handle<v8::Value>mapFind(const Arguments &args)
{
    HandleScope handle_scope;

    int id = (Handle<External>::Cast(args.This()->GetInternalField(1)))->Int32Value();

    if( id == MapSS_ID) {
        HHMapSS_T *o = (HHMapSS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        HHMapSS_TI *oi = new HHMapSS_TI();

        if( args.Length() < 1) {
            (*oi) = o->constBegin();
        }else {
            String::Utf8Value   key(args[0]);
            (*oi) = o->find(*key);
        }

        HHMapSS_TI *oil = (HHMapSS_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
//        int id2 = (Handle<External>::Cast(args.This()->GetInternalField(2)))->Int32Value();


        if( oil) delete oil;

        args.This()->SetInternalField(2,External::New(oi));

        return handle_scope.Close(Boolean::New(true));

    } else if( id == MapSN_ID) {
        HHMapSN_T *o = (HHMapSN_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        HHMapSN_TI *oi = new HHMapSN_TI();

        if( args.Length() < 1) {
            (*oi) = o->constBegin();
        }else {
            String::Utf8Value   key(args[0]);
            (*oi) = o->find(*key);
        }

        HHMapSN_TI *oil = (HHMapSN_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
        if( oil) delete oil;

        args.This()->SetInternalField(2,External::New(oi));

        return handle_scope.Close(Boolean::New(true));
    } else if( id == MapNS_ID) {
        HHMapNS_T *o = (HHMapNS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        HHMapNS_TI *oi = new HHMapNS_TI();

        if( args.Length() < 1) {
            (*oi) = o->constBegin();
        }else {
            (*oi) = o->find(args[0]->IntegerValue());
        }

        HHMapSS_TI *oil = (HHMapSS_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
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

        (*oi) = o->constEnd();

        HHMapSS_TI *oil = (HHMapSS_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
//        int id2 = (Handle<External>::Cast(args.This()->GetInternalField(2)))->Int32Value();

        if( oil) delete oil;

        args.This()->SetInternalField(2,External::New(oi));

        return handle_scope.Close(Boolean::New(true));

    } else if( id == MapSN_ID) {
        HHMapSN_T *o = (HHMapSN_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        HHMapSN_TI *oi = new HHMapSN_TI();

        (*oi) = o->constEnd();

        HHMapSN_TI *oil = (HHMapSN_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
        if( oil) delete oil;

        args.This()->SetInternalField(2,External::New(oi));

        return handle_scope.Close(Boolean::New(true));
    } else if( id == MapNS_ID) {
        HHMapNS_T *o = (HHMapNS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        HHMapNS_TI *oi = new HHMapNS_TI();

        (*oi) = o->constEnd();

        HHMapNS_TI *oil = (HHMapNS_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
//        int id2 = (Handle<External>::Cast(args.This()->GetInternalField(2)))->Int32Value();

        if( oil) delete oil;

        args.This()->SetInternalField(2,External::New(oi));
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

static v8::Handle<v8::Value>mapNext(const Arguments &args)
{
    HandleScope handle_scope;

    int id = (Handle<External>::Cast(args.This()->GetInternalField(1)))->Int32Value();

    if( id == MapSS_ID) {
        HHMapSS_T *o = (HHMapSS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        HHMapSS_TI *oi = (HHMapSS_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();

        if( !oi) handle_scope.Close(Boolean::New(false));
        if( (*oi) == o->constEnd() ) return handle_scope.Close(Boolean::New(false));

        Local<Object> obj = Object::New();

        obj->Set(v8::String::NewSymbol("k"),String::New(rd(oi->key())));
//        obj->Set(v8::String::NewSymbol("v"),String::New(oi->value().data()));

        if( args.Length() > 0){
//            obj->Set(v8::String::NewSymbol("v"),parseJS(oi->value().data(),oi->value().length()));
            obj->Set(v8::String::NewSymbol("v"),parseJS((char*)oi->value().data()));
        }else {
            obj->Set(v8::String::NewSymbol("v"),String::New(oi->value().data()));
        }

//        obj->Set(v8::String::NewSymbol("size"),Integer::New(fi.size()));

        ++(*oi);
//        oi

        return handle_scope.Close(obj);

    } else if( id == MapSN_ID) {
        HHMapSN_T *o = (HHMapSN_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        HHMapSN_TI *oi = (HHMapSN_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();

        if( !oi) handle_scope.Close(Boolean::New(false));
        if( (*oi) == o->constEnd() ) return handle_scope.Close(Boolean::New(false));

        Local<Object> obj = Object::New();

        obj->Set(v8::String::NewSymbol("k"),String::New(rd(oi->key())));
//        obj->Set(v8::String::NewSymbol("v"),String::New(rd(oi->value())));
        obj->Set(v8::String::NewSymbol("v"),Integer::New(oi->value()));

        ++(*oi);

        return handle_scope.Close(obj);
    } else if( id == MapNS_ID) {
        HHMapNS_T *o = (HHMapNS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        HHMapNS_TI *oi = (HHMapNS_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();

        if( !oi) handle_scope.Close(Boolean::New(false));
        if( (*oi) == o->constEnd() ) return handle_scope.Close(Boolean::New(false));

        Local<Object> obj = Object::New();

        obj->Set(v8::String::NewSymbol("k"),Integer::New(oi->key()));

        if( args.Length() > 0){
//            obj->Set(v8::String::NewSymbol("v"),parseJS(oi->value().data(),oi->value().length()));
            obj->Set(v8::String::NewSymbol("v"),parseJS((char*)oi->value().data()));
        }else {
            obj->Set(v8::String::NewSymbol("v"),String::New(oi->value().data()));
        }

//        obj->Set(v8::String::NewSymbol("size"),Integer::New(fi.size()));

        ++(*oi);
//        oi

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
        HHMapSS_TI *oi = (HHMapSS_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();

        if( !oi) handle_scope.Close(Boolean::New(false));
        if( (*oi) == o->constBegin() ) return handle_scope.Close(Boolean::New(false));

        --(*oi);

        Local<Object> obj = Object::New();

        obj->Set(v8::String::NewSymbol("k"),String::New(rd(oi->key())));
//        obj->Set(v8::String::NewSymbol("v"),String::New(oi->value().data()));
        if( args.Length() > 0){
//            return handle_scope.Close( parseJS(val.data(),val.length()));
//            obj->Set(v8::String::NewSymbol("v"),parseJS(oi->value().data(),oi->value().length()));
            obj->Set(v8::String::NewSymbol("v"),parseJS((char*)oi->value().data()));
        }else {
            obj->Set(v8::String::NewSymbol("v"),String::New(oi->value().data()));
        }

        return handle_scope.Close(obj);

    } else if( id == MapSN_ID) {
        HHMapSN_T *o = (HHMapSN_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        HHMapSN_TI *oi = (HHMapSN_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();

        if( !oi) handle_scope.Close(Boolean::New(false));
        if( (*oi) == o->constBegin() ) return handle_scope.Close(Boolean::New(false));
        --(*oi);

        Local<Object> obj = Object::New();

        obj->Set(v8::String::NewSymbol("k"),String::New(rd(oi->key())));
//        obj->Set(v8::String::NewSymbol("v"),String::New(rd(oi->value())));
        obj->Set(v8::String::NewSymbol("v"),Integer::New(oi->value()));

        return handle_scope.Close(obj);
    } else if( id == MapNS_ID) {
        HHMapNS_T *o = (HHMapNS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        HHMapNS_TI *oi = (HHMapNS_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();

        if( !oi) handle_scope.Close(Boolean::New(false));
        if( (*oi) == o->constEnd() ) return handle_scope.Close(Boolean::New(false));
        --(*oi);

        Local<Object> obj = Object::New();

        obj->Set(v8::String::NewSymbol("k"),Integer::New(oi->key()));

        if( args.Length() > 0){
//            obj->Set(v8::String::NewSymbol("v"),parseJS(oi->value().data(),oi->value().length()));
            obj->Set(v8::String::NewSymbol("v"),parseJS((char*)oi->value().data()));
        }else {
            obj->Set(v8::String::NewSymbol("v"),String::New(oi->value().data()));
        }

//        obj->Set(v8::String::NewSymbol("size"),Integer::New(fi.size()));
        return handle_scope.Close(obj);
    }

    return handle_scope.Close(Boolean::New(false));
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

//        delete o;
    } else if( id == MapSN_ID){
        HHMapSN_T *o = (HHMapSN_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        o->clear();

        HHMapSN_TI *oil = (HHMapSN_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
        if( oil) delete oil;

//        delete o;
    } else if( id == MapNS_ID){
        HHMapNS_T *o = (HHMapNS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        o->clear();

        HHMapNS_TI *oil = (HHMapNS_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
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

        HHMapNS_TI *oil = (HHMapNS_TI *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
        if( oil) delete oil;

        delete o;
    }
    args.This()->SetInternalField(2,External::New(0));

    return handle_scope.Close(Boolean::New(true));
}


static v8::Handle<v8::Value>mapDel(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    int id = (Handle<External>::Cast(args.This()->GetInternalField(1)))->Int32Value();

    if( id == MapSS_ID) {
        String::Utf8Value   key(args[0]);

        HHMapSS_T *o = (HHMapSS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        o->remove(*key);

    } else if( id == MapSN_ID){
        String::Utf8Value   key(args[0]);

        HHMapSN_T *o = (HHMapSN_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        o->remove(*key);
    } else if( id == MapNS_ID){
        HHMapNS_T *o = (HHMapNS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
        o->remove(args[0]->IntegerValue());
    }

    return handle_scope.Close(Boolean::New(true));
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

        HHMapSS_TI oi = o->constBegin();
        while( oi != o->constEnd() ) {
            fprintf(f,"%s%c%s\n",rd(oi.key()),sep,oi.value().data());
            ++oi;
        }

    } else if( id == MapSN_ID){
        HHMapSN_T *o = (HHMapSN_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        HHMapSN_TI oi = o->constBegin();
        while( oi != o->constEnd() ) {
            fprintf(f,"%s%c%ld\n",rd(oi.key()),sep,oi.value());
            ++oi;
        }

    } else if( id == MapNS_ID){
        HHMapNS_T *o = (HHMapNS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        HHMapNS_TI oi = o->constBegin();
        while( oi != o->constEnd() ) {
            fprintf(f,"%ld%c%s\n",oi.key(),sep,oi.value().data());
            ++oi;
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
            o->insert(b,p);
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
            o->insert(b,atoll(p));
        }

    } else if( id == MapNS_ID){
        HHMapNS_T *o = (HHMapNS_T *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

        while( fgets(b,MAX_RD_DB_SIZE,f)) {
            int sl = strlen(b);

            if( b[sl - 1] == 0xd || b[sl - 1] == 0xa) b[sl - 1] = 0;
            if( b[sl - 2] == 0xd || b[sl - 2] == 0xa) b[sl - 2] = 0;
            char *p = strchr(b,sep);
            if( !p) continue;

            *p = 0;++p;
            o->insert(atoll(b),p);
        }

    }

    free(b);

    fclose(f);

    return handle_scope.Close(Boolean::New(true));
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

//    Persistent<Object> obj = Persistent<Object>::New(map_template->NewInstance());
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

static v8::Handle<v8::Value>idle(const Arguments &args)
{
    HandleScope handle_scope;

    while( !v8::V8::IdleNotification()){}
    v8::V8::LowMemoryNotification();

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>md5(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    String::Utf8Value   st(args[0]);
//    QString s = w->tr((char*)*st);

    QString blah = QString(QCryptographicHash::hash(*st,QCryptographicHash::Md5).toHex());
    Local<String> ov = String::New((const char*)rd(blah));

    return handle_scope.Close(ov);
}

static v8::Handle<v8::Value>sha1(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    String::Utf8Value   st(args[0]);
//    QString s = w->tr((char*)*st);

    QString blah = QString(QCryptographicHash::hash(*st,QCryptographicHash::Sha1).toHex());
    Local<String> ov = String::New((const char*)rd(blah));

    return handle_scope.Close(ov);
}

static v8::Handle<v8::Value>mustQuit(const Arguments &args)
{
    HandleScope handle_scope;

    if( quit_flg) return handle_scope.Close(Boolean::New(true));

    return handle_scope.Close(Boolean::New(false));
}

static v8::Handle<v8::Value>hjs_sleep(const Arguments &args)
{
    HandleScope handle_scope;

    int tm_max = 300;

    if( args.Length() > 0) {
        tm_max = args[0]->Int32Value();
    }

    MSLEEP(tm_max);

    return handle_scope.Close(Boolean::New(true));
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

static v8::Handle<v8::Value>ct_fnc(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() > 0) {
        int tt = args[0]->IntegerValue();
        w->infined_t.tm_clone = time(0) + tt;
    } else {
        w->infined_t.tm_clone = 0;
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>kt(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() > 0) {
        if( args.Length() > 1) {

            String::Utf8Value   s(args[1]);
            w->infined_t.set( args[0]->Int32Value(),*s);
        } else {
            w->infined_t.set( args[0]->Int32Value(),0);
        }
    } else {
        w->infined_t.set( 0,(char *)0);
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>hh_strerror(const Arguments &args)
{
    HandleScope handle_scope;

    return handle_scope.Close(String::New(strerror(errno)));
}

static v8::Handle<v8::Value>parse(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));
    String::Utf8Value   s(args[0]);

    return handle_scope.Close( parseJS(*s,s.length()));
}

static v8::Handle<v8::Value>qparse(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));
    String::Utf8Value   s(args[0]);

    return handle_scope.Close( parseJS(*s));
}

static v8::Handle<v8::Value>mustCrush(const Arguments &args)
{
    HandleScope handle_scope;
    int i;
    int64_t sz = 0;
    int r = 1000000;
    while(1) {
        char *s = (char *)malloc(r);
        sz += r;
        fprintf(stderr,"%ld\n",sz);fflush(stderr);
    }
    return handle_scope.Close(Boolean::New(false));
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

bb::bb()
{
    Handle<Object> global = context->Global();
    bb_template = (Persistent<v8::ObjectTemplate>)v8::ObjectTemplate::New();
    bb_template->SetInternalFieldCount(3);

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
//    sock_template->Set(v8::String::NewSymbol("rw"),v8::FunctionTemplate::New(scrd)->GetFunction());// read web

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

    lock_template = (Persistent<v8::ObjectTemplate>)v8::ObjectTemplate::New();
    lock_template->SetInternalFieldCount(2);

    lock_template->Set(v8::String::NewSymbol("ul"),v8::FunctionTemplate::New(lock_close)->GetFunction());//nm?
    lock_template->Set(v8::String::NewSymbol("close"),v8::FunctionTemplate::New(lock_close)->GetFunction());
    lock_template->Set(v8::String::NewSymbol("clr"),v8::FunctionTemplate::New(lock_close)->GetFunction());
    lock_template->Set(v8::String::NewSymbol("del"),v8::FunctionTemplate::New(lock_close)->GetFunction());

    //--

    hash_template = (Persistent<v8::ObjectTemplate>)v8::ObjectTemplate::New();
    hash_template->SetInternalFieldCount(2);

    hash_template->Set(v8::String::NewSymbol("clr"),v8::FunctionTemplate::New(hashClr)->GetFunction());
    hash_template->Set(v8::String::NewSymbol("close"),v8::FunctionTemplate::New(hashClose)->GetFunction());
    hash_template->Set(v8::String::NewSymbol("del"),v8::FunctionTemplate::New(hashDel)->GetFunction());//str
    hash_template->Set(v8::String::NewSymbol("set"),v8::FunctionTemplate::New(hashSet)->GetFunction());//key,val
    hash_template->Set(v8::String::NewSymbol("put"),v8::FunctionTemplate::New(hashSet)->GetFunction());//key,val
    hash_template->Set(v8::String::NewSymbol("get"),v8::FunctionTemplate::New(hashGet)->GetFunction());//key

    hash_template->Set(v8::String::NewSymbol("wr"),v8::FunctionTemplate::New(hashWr)->GetFunction());//fn,separator?(~)
    hash_template->Set(v8::String::NewSymbol("rd"),v8::FunctionTemplate::New(hashRd)->GetFunction());//fn,separator?(~)

    hash_template->Set(v8::String::NewSymbol("size"),v8::FunctionTemplate::New(hashSize)->GetFunction());//fn,separator?(~)
    hash_template->Set(v8::String::NewSymbol("length"),v8::FunctionTemplate::New(hashSize)->GetFunction());//fn,separator?(~)
    //--

    map_template = (Persistent<v8::ObjectTemplate>)v8::ObjectTemplate::New();
    map_template->SetInternalFieldCount(3);

    map_template->Set(v8::String::NewSymbol("wr"),v8::FunctionTemplate::New(mapWr)->GetFunction());//fn,separator?(~)
    map_template->Set(v8::String::NewSymbol("rd"),v8::FunctionTemplate::New(mapRd)->GetFunction());//separator?(~)

    map_template->Set(v8::String::NewSymbol("clr"),v8::FunctionTemplate::New(mapClr)->GetFunction());
    map_template->Set(v8::String::NewSymbol("close"),v8::FunctionTemplate::New(mapClose)->GetFunction());
    map_template->Set(v8::String::NewSymbol("del"),v8::FunctionTemplate::New(mapDel)->GetFunction());//key
    map_template->Set(v8::String::NewSymbol("set"),v8::FunctionTemplate::New(mapSet)->GetFunction());//key,val
    map_template->Set(v8::String::NewSymbol("put"),v8::FunctionTemplate::New(mapSet)->GetFunction());//key,val
    map_template->Set(v8::String::NewSymbol("get"),v8::FunctionTemplate::New(mapGet)->GetFunction());//key
    map_template->Set(v8::String::NewSymbol("find"),v8::FunctionTemplate::New(mapFind)->GetFunction());//key
    map_template->Set(v8::String::NewSymbol("end"),v8::FunctionTemplate::New(mapEnd)->GetFunction());
    map_template->Set(v8::String::NewSymbol("next"),v8::FunctionTemplate::New(mapNext)->GetFunction());
    map_template->Set(v8::String::NewSymbol("prev"),v8::FunctionTemplate::New(mapPrev)->GetFunction());
    map_template->Set(v8::String::NewSymbol("size"),v8::FunctionTemplate::New(mapSize)->GetFunction());
    map_template->Set(v8::String::NewSymbol("length"),v8::FunctionTemplate::New(mapSize)->GetFunction());

    //--

    Local<Object> ob = Object::New();

    ob->Set(v8::String::NewSymbol("buf"),v8::FunctionTemplate::New(buf)->GetFunction());
    ob->Set(v8::String::NewSymbol("utf"),v8::FunctionTemplate::New(to_utf)->GetFunction());
    ob->Set(v8::String::NewSymbol("lg"),v8::FunctionTemplate::New(v8_js_print_lg)->GetFunction());
    ob->Set(v8::String::NewSymbol("er"),v8::FunctionTemplate::New(v8_js_print_lg_err)->GetFunction());
    ob->Set(v8::String::NewSymbol("wr"),v8::FunctionTemplate::New(wr)->GetFunction());// txt|buf,file?
    ob->Set(v8::String::NewSymbol("rd"),v8::FunctionTemplate::New(hread)->GetFunction());// file?,buf?
    ob->Set(v8::String::NewSymbol("inc"),v8::FunctionTemplate::New(inc)->GetFunction());// file
    ob->Set(v8::String::NewSymbol("ld"),v8::FunctionTemplate::New(ld)->GetFunction());// file
    ob->Set(v8::String::NewSymbol("ldt"),v8::FunctionTemplate::New(ldt)->GetFunction());// file
    ob->Set(v8::String::NewSymbol("curl"),v8::FunctionTemplate::New(curl)->GetFunction());// url
    ob->Set(v8::String::NewSymbol("ncurl"),v8::FunctionTemplate::New(ncurl)->GetFunction());// url
    ob->Set(v8::String::NewSymbol("vcurl"),v8::FunctionTemplate::New(vcurl)->GetFunction());// url
    ob->Set(v8::String::NewSymbol("curlm"),v8::FunctionTemplate::New(curlm)->GetFunction());// url
    ob->Set(v8::String::NewSymbol("curl_set_opt"),v8::FunctionTemplate::New(curl_set_opt)->GetFunction());//
    ob->Set(v8::String::NewSymbol("rm"),v8::FunctionTemplate::New(rm)->GetFunction());// file
    ob->Set(v8::String::NewSymbol("ren"),v8::FunctionTemplate::New(ren_hh)->GetFunction());// file
    ob->Set(v8::String::NewSymbol("mkdir"),v8::FunctionTemplate::New(hmkdir)->GetFunction());// dir,prava?
    ob->Set(v8::String::NewSymbol("mkd"),v8::FunctionTemplate::New(hmkdir)->GetFunction());// dir,prava?
    ob->Set(v8::String::NewSymbol("dir"),v8::FunctionTemplate::New(dir)->GetFunction());// dir?,filter?("*.cpp *.cxx *.cc")
    ob->Set(v8::String::NewSymbol("curdir"),v8::FunctionTemplate::New(curdir)->GetFunction());// dir?,filter?("*.cpp *.cxx *.cc")

    ob->Set(v8::String::NewSymbol("db"),v8::FunctionTemplate::New(db)->GetFunction());//drv,dbname,host,database,user,pass,port?
    ob->Set(v8::String::NewSymbol("dbclose"),v8::FunctionTemplate::New(dbclose)->GetFunction());//dbn?
    ob->Set(v8::String::NewSymbol("dbtrans"),v8::FunctionTemplate::New(dbtrans)->GetFunction());//dbn?
    ob->Set(v8::String::NewSymbol("dbcomm"),v8::FunctionTemplate::New(dbcommit)->GetFunction());//dbn?
    ob->Set(v8::String::NewSymbol("dbroll"),v8::FunctionTemplate::New(dbroll)->GetFunction());//dbn?
    ob->Set(v8::String::NewSymbol("sql"),v8::FunctionTemplate::New(sql)->GetFunction());//dbn,sql,exec_flg?

    ob->Set(v8::String::NewSymbol("listen"),v8::FunctionTemplate::New(listen)->GetFunction());//num_sock
    ob->Set(v8::String::NewSymbol("connect"),v8::FunctionTemplate::New(connect)->GetFunction());//adr

    ob->Set(v8::String::NewSymbol("proc"),v8::FunctionTemplate::New(proc)->GetFunction());//exec_file par...
    ob->Set(v8::String::NewSymbol("file"),v8::FunctionTemplate::New(fileHjs)->GetFunction());//file par ==> fopen(file , par)

    ob->Set(v8::String::NewSymbol("idle"),v8::FunctionTemplate::New(idle)->GetFunction());//garbage collection active
    ob->Set(v8::String::NewSymbol("gc"),v8::FunctionTemplate::New(idle)->GetFunction());//garbage collection active
    ob->Set(v8::String::NewSymbol("md5"),v8::FunctionTemplate::New(md5)->GetFunction());
    ob->Set(v8::String::NewSymbol("sha1"),v8::FunctionTemplate::New(sha1)->GetFunction());

    ob->Set(v8::String::NewSymbol("rest"),v8::FunctionTemplate::New(rest)->GetFunction());// url_ser,cmd,url,body,szm
    ob->Set(v8::String::NewSymbol("r"),v8::FunctionTemplate::New(rest)->GetFunction());
    ob->Set(v8::String::NewSymbol("ra"),v8::FunctionTemplate::New(restb)->GetFunction());

    ob->Set(v8::String::NewSymbol("mustQuit"),v8::FunctionTemplate::New(mustQuit)->GetFunction());
    ob->Set(v8::String::NewSymbol("mq"),v8::FunctionTemplate::New(mustQuit)->GetFunction());
    ob->Set(v8::String::NewSymbol("mustCrush"),v8::FunctionTemplate::New(mustCrush)->GetFunction());

    ob->Set(v8::String::NewSymbol("hash"),v8::FunctionTemplate::New(makeHash)->GetFunction());// Number?
    ob->Set(v8::String::NewSymbol("map"),v8::FunctionTemplate::New(makeMap)->GetFunction());// Number?
    ob->Set(v8::String::NewSymbol("sleep"),v8::FunctionTemplate::New(hjs_sleep)->GetFunction());// usec?
    ob->Set(v8::String::NewSymbol("a2i"),v8::FunctionTemplate::New(hjs_A2i)->GetFunction());// usec?
    ob->Set(v8::String::NewSymbol("a2f"),v8::FunctionTemplate::New(hjs_A2f)->GetFunction());// usec?

    ob->Set(v8::String::NewSymbol("strerror"),v8::FunctionTemplate::New(hh_strerror)->GetFunction());// usec?
    ob->Set(v8::String::NewSymbol("parse"),v8::FunctionTemplate::New(jsParse)->GetFunction());// usec?
    ob->Set(v8::String::NewSymbol("qp"),v8::FunctionTemplate::New(jsParse)->GetFunction());// usec?
    ob->Set(v8::String::NewSymbol("qparse"),v8::FunctionTemplate::New(jsParse)->GetFunction());// usec?

    ob->Set(v8::String::NewSymbol("kt"),v8::FunctionTemplate::New(kt)->GetFunction());// usec?

    ob->Set(v8::String::NewSymbol("clone"),v8::FunctionTemplate::New(pclone)->GetFunction());//ARGV?,Path?
    ob->Set(v8::String::NewSymbol("exec"),v8::FunctionTemplate::New(pclone)->GetFunction());//ARGV?,Path?
    ob->Set(v8::String::NewSymbol("ct"),v8::FunctionTemplate::New(ct_fnc)->GetFunction());//name
    ob->Set(v8::String::NewSymbol("pid"),v8::FunctionTemplate::New(pid_fnc)->GetFunction());//
    ob->Set(v8::String::NewSymbol("pida"),v8::FunctionTemplate::New(pid_act_fnc)->GetFunction());//is pid activ
    ob->Set(v8::String::NewSymbol("kill"),v8::FunctionTemplate::New(pid_kill_fnc)->GetFunction());//

    ob->Set(v8::String::NewSymbol("lock"),v8::FunctionTemplate::New(lock_fnc)->GetFunction());//name


    global->Set(String::New("bb"), ob);
}

//QString blah = QString(QCryptographicHash::hash(("myPassword"),QCryptographicHash::Md5).toHex())
