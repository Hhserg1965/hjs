#include "mcachjs.h"
#include "time.h"
//#include <mysql/mysql.h>

extern v8::Persistent<v8::Context> context;
extern char   *glb_arr[];
//Persistent<v8::ObjectTemplate> cvw_template;
//Persistent<v8::ObjectTemplate> mat_template;
//static v8::Handle<v8::ObjectTemplate>mat_t();


enum{
MEMCACHED_ID=22200,MEMCACHED_POOL_ID
};

enum{
Buf_ID=20000,
};


int init_cnt=0;

v8::Handle<v8::Value>(*bb_buf)(char *b,int sz);
v8::Handle<v8::Value>out_buf(char *b,int sz);
extern "C" {
#include <libmemcached/memcached.h>
#include <libmemcached/util.h>
}

static void objectWeakCallback(v8::Persistent<v8::Value> obj, void* parameter)
{
    v8::Persistent<v8::Object> extObj = v8::Persistent<v8::Object>::Cast(obj);
    void * par = (void *)(Handle<External>::Cast(extObj->GetInternalField(0)))->Value();
    v8::Handle<v8::External> extObjID = v8::Handle<v8::External>::Cast(extObj->GetInternalField(1));

    switch( extObjID->Int32Value())
    {
        case MEMCACHED_ID:
        {
           memcached_st *o = (memcached_st *) par;

            if( o) {
                memcached_free(o);
            }
            break;
        }
        case MEMCACHED_POOL_ID:
        {
           memcached_pool_st *o = (memcached_pool_st *) par;

            if( o) {
                memcached_pool_destroy(o);
            }
            break;
        }

    }

    obj.ClearWeak();
//    obj.Dispose();
    obj.Clear();
}

static v8::Handle<v8::Value>mc_mget(const Arguments &args) // [keys],gkey?
{
    HandleScope handle_scope;

    if( args.Length() < 1) return Boolean::New(false);

    Local<Array> keysa = Local<Array>::Cast(args[0]);

    char **keys = (char **)malloc(keysa->Length() * sizeof(char *));
    size_t *key_length = (size_t *)malloc(keysa->Length() * sizeof(size_t *));

    for (unsigned j = 0; j < keysa->Length(); j++) {
        String::Utf8Value nm(keysa->Get(Integer::New(j)));
//        fprintf(stderr,"keys :: %s\n",*nm);
        char *p = (char *) malloc(nm.length()+1);
        memcpy(p,*nm,nm.length());
        p[nm.length()] = 0;
        keys[j] = p;
        key_length[j] = nm.length();
//        fprintf(stderr,"keys :: %s %d\n",keys[j],key_length[j]);
    }

    char gkey[300] = "";

    memcached_st * memc = (memcached_st *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

    memcached_return_t rc = MEMCACHED_SUCCESS;

    if( args.Length() >= 2) {
        if( args[1]->IsString() ) {
            String::Utf8Value keyg(args[1]);
            strcpy(gkey,*keyg);
        }
    }

    if( !strlen(gkey)){
        rc = memcached_mget(memc, keys, key_length, keysa->Length());
    }else{
        rc = memcached_mget_by_key(memc, gkey, strlen(gkey), keys, key_length, keysa->Length());
    }

    if( rc) {
        if( rc != MEMCACHED_NOTFOUND) fprintf(stderr,"mget ERROR :: %d %s \n",rc,memcached_strerror(memc,rc));
        return handle_scope.Close(Boolean::New(false));
    }

    for(unsigned j = 0; j < keysa->Length(); j++) {
        if( keys[j]) free(keys[j]);
    }

    free(keys);
    free(key_length);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>mc_get(const Arguments &args)//key,gkey?
{
    HandleScope handle_scope;

    if( args.Length() < 1) return Boolean::New(false);
    String::Utf8Value key(args[0]);
    char gkey[300] = "";

    memcached_st * memc = (memcached_st *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

    uint64_t init = 0;
    uint64_t value = 0;
    memcached_return_t rc = MEMCACHED_SUCCESS;
    size_t value_length;
    char *zn = 0;

    if( args.Length() >= 2) {
        if( args[1]->IsString() ) {
            String::Utf8Value keyg(args[1]);
            strcpy(gkey,*keyg);
        }
    }

    if( !strlen(gkey)){
        zn =  memcached_get(memc,*key, strlen(*key), &value_length, 0,&rc);
    }else{
//        rc = memcached_increment_with_initial_by_key(memc, gkey, strlen(gkey), *key, strlen(*key),(uint64_t)1,(uint64_t) init, (time_t)0, &value);
        zn =  memcached_get_by_key(memc, gkey, strlen(gkey),*key, strlen(*key), &value_length, 0,&rc);
    }

    if( rc) {
        if( rc != MEMCACHED_NOTFOUND)
            fprintf(stderr,"get ERROR :: %d %s \n",rc,memcached_strerror(memc,rc));
        return handle_scope.Close(Boolean::New(false));
    }

//    fprintf(stderr,"mc_get :: (%s) %d\n",zn,value_length);

    Local<String> st = String::New(zn,value_length);
    free(zn);

    return handle_scope.Close(st);
}

static v8::Handle<v8::Value>mc_fetch(const Arguments &args)
{
    HandleScope handle_scope;

    memcached_st * memc = (memcached_st *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
    memcached_return_t rc = MEMCACHED_SUCCESS;

    memcached_result_st *result;
//     = memcached_result_create(memc, NULL);
//    if( !result) return Boolean::New(false);

    result = memcached_fetch_result(memc, NULL, &rc);
    if( !result ) { return Boolean::New(false);}

    Handle<Object> obj = Object::New();

    obj->Set(v8::String::NewSymbol("key"),String::New(memcached_result_key_value(result),memcached_result_key_length(result)));
    obj->Set(v8::String::NewSymbol("val"),String::New(memcached_result_value(result),memcached_result_length(result)));
    obj->Set(v8::String::NewSymbol("cas"),Integer::New(memcached_result_cas(result)));

    memcached_result_free(result);

    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>mc_fetchb(const Arguments &args)
{
    HandleScope handle_scope;

    memcached_st * memc = (memcached_st *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
    memcached_return_t rc = MEMCACHED_SUCCESS;

    memcached_result_st *result;
//     = memcached_result_create(memc, NULL);
//    if( !result) return Boolean::New(false);

    result = memcached_fetch_result(memc, NULL, &rc);
    if( !result ) { return Boolean::New(false);}

    Handle<Object> obj = Object::New();

    obj->Set(v8::String::NewSymbol("key"),String::New(memcached_result_key_value(result),memcached_result_key_length(result)));
    obj->Set(v8::String::NewSymbol("val"),(*bb_buf)((char *)memcached_result_value(result),memcached_result_length(result)));
    obj->Set(v8::String::NewSymbol("cas"),Integer::New(memcached_result_cas(result)));

    memcached_result_free(result);

    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>mc_getb(const Arguments &args)//key,gkey?
{
    HandleScope handle_scope;

    if( args.Length() < 1) return Boolean::New(false);
    String::Utf8Value key(args[0]);
    char gkey[300] = "";

    memcached_st * memc = (memcached_st *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

    uint64_t init = 0;
    uint64_t value = 0;
    memcached_return_t rc = MEMCACHED_SUCCESS;
    size_t value_length;
    char *zn = 0;

    if( args.Length() >= 2) {
        if( args[1]->IsString() ) {
            String::Utf8Value keyg(args[1]);
            strcpy(gkey,*keyg);
        }
    }

    if( !strlen(gkey)){
        zn =  memcached_get(memc,*key, strlen(*key), &value_length, 0,&rc);
    }else{
//        rc = memcached_increment_with_initial_by_key(memc, gkey, strlen(gkey), *key, strlen(*key),(uint64_t)1,(uint64_t) init, (time_t)0, &value);
        zn =  memcached_get_by_key(memc, gkey, strlen(gkey),*key, strlen(*key), &value_length, 0,&rc);
    }

    if( rc) {
        if( rc != MEMCACHED_NOTFOUND)
            fprintf(stderr,"getb ERROR :: %d %s \n",rc,memcached_strerror(memc,rc));
        return handle_scope.Close(Boolean::New(false));
    }

//    fprintf(stderr,"mc_get :: (%s) %d\n",zn,value_length);

    v8::Handle<v8::Value> o = (*bb_buf)(zn,value_length);
    free(zn);

    return handle_scope.Close(o);
}

static v8::Handle<v8::Value>mc_exist(const Arguments &args)//key,gkey?
{
    HandleScope handle_scope;

    if( args.Length() < 1) return Boolean::New(false);
    String::Utf8Value key(args[0]);
    char gkey[300] = "";

    memcached_st * memc = (memcached_st *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

//    uint64_t init = 0;
//    uint64_t value = 0;
    memcached_return_t rc = MEMCACHED_SUCCESS;
//    size_t value_length;
    char *zn = 0;

    if( args.Length() >= 2) {
        if( args[1]->IsString() ) {
            String::Utf8Value keyg(args[1]);
            strcpy(gkey,*keyg);
        }
    }

    if( !strlen(gkey)){
        rc = memcached_exist(memc,*key, strlen(*key));
    }else{
        rc = memcached_exist_by_key(memc, gkey, strlen(gkey),*key, strlen(*key));
    }

    if( rc == MEMCACHED_NOTFOUND ) {
//        fprintf(stderr,"exist ERROR :: %s \n",memcached_strerror(memc,rc));
        return handle_scope.Close(Boolean::New(false));
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>mc_set(const Arguments &args)//key,val,gkey?|exp?,exp?
{
    HandleScope handle_scope;

    if( args.Length() < 2) return Boolean::New(false);
    String::Utf8Value key(args[0]);
    String::Utf8Value vl(args[1]);
    char *val = *vl;
    size_t val_l = strlen(val);

    if( args[1]->IsObject() ) {
        Local<Object> obj = args[1]->ToObject();
        Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
        hBuf* o = (hBuf*)field->Value();
        int32_t id = obj->GetInternalField(1)->Int32Value();
        if( id != Buf_ID) return Boolean::New(false);

        val = o->p + o->beg;
        val_l = o->end - o->beg;
    }

    char gkey[300] = "";
    time_t exp = 0;

    memcached_st * memc = (memcached_st *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

    memcached_return_t rc = MEMCACHED_SUCCESS;
    size_t value_length;
    char *zn = 0;

    if( args.Length() > 2) {
        if( args[2]->IsNumber() ) {
            exp = time(0) + (args[2]->IntegerValue());
        }

        if( args[2]->IsString() ) {
            String::Utf8Value keyg(args[2]);
            strcpy(gkey,*keyg);
        }
    }
    if( args.Length() > 3) {
        if( args[3]->IsNumber() ) {
            exp = time(0) + (args[3]->IntegerValue());
        }
    }

    if( !strlen(gkey)){
        rc = memcached_set(memc,*key, strlen(*key), val, val_l,exp, 0);
    }else{
        rc = memcached_set_by_key(memc, gkey, strlen(gkey),*key, strlen(*key), val, val_l,exp, 0);
    }

    if( rc) {
        fprintf(stderr,"set ERROR :: %d %s \n",rc,memcached_strerror(memc,rc));
        return handle_scope.Close(Boolean::New(false));
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>mc_cas(const Arguments &args)//key,val,gkey?|cas?,cas?
{
    HandleScope handle_scope;

    if( args.Length() < 2) return Boolean::New(false);
    String::Utf8Value key(args[0]);
    String::Utf8Value vl(args[1]);
    char *val = *vl;
    size_t val_l = strlen(val);

    if( args[1]->IsObject() ) {
        Local<Object> obj = args[1]->ToObject();
        Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
        hBuf* o = (hBuf*)field->Value();
        int32_t id = obj->GetInternalField(1)->Int32Value();
        if( id != Buf_ID) return Boolean::New(false);

        val = o->p + o->beg;
        val_l = o->end - o->beg;
    }

    char gkey[300] = "";
    time_t exp = 0;
    int64_t cas = 0;

    memcached_st * memc = (memcached_st *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

    memcached_return_t rc = MEMCACHED_SUCCESS;
    size_t value_length;
    char *zn = 0;

    if( args.Length() > 2) {
        if( args[2]->IsNumber() ) {
            cas = args[2]->IntegerValue();
        }

        if( args[2]->IsString() ) {
            String::Utf8Value keyg(args[2]);
            strcpy(gkey,*keyg);
        }
    }
    if( args.Length() > 3) {
        if( args[3]->IsNumber() ) {
            cas = args[3]->IntegerValue() ;
        }
    }

    if( !strlen(gkey)){
        rc = memcached_set(memc,*key, strlen(*key), val, val_l,exp, 0);
    }else{
        rc = memcached_set_by_key(memc, gkey, strlen(gkey),*key, strlen(*key), val, val_l,exp, 0);
    }

    if( rc) {
        fprintf(stderr,"cas ERROR :: %d %s \n",rc,memcached_strerror(memc,rc));
        return handle_scope.Close(Boolean::New(false));
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>mc_add(const Arguments &args)//key,val,gkey?|exp?,exp?
{
    HandleScope handle_scope;

    if( args.Length() < 2) return Boolean::New(false);
    String::Utf8Value key(args[0]);
    String::Utf8Value vl(args[1]);
    char *val = *vl;
    size_t val_l = strlen(val);

    if( args[1]->IsObject() ) {
        Local<Object> obj = args[1]->ToObject();
        Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
        hBuf* o = (hBuf*)field->Value();
        int32_t id = obj->GetInternalField(1)->Int32Value();
        if( id != Buf_ID) return Boolean::New(false);

        val = o->p + o->beg;
        val_l = o->end - o->beg;
    }

    char gkey[300] = "";
    time_t exp = 0;

    memcached_st * memc = (memcached_st *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

    memcached_return_t rc = MEMCACHED_SUCCESS;
    size_t value_length;
    char *zn = 0;

    if( args.Length() > 2) {
        if( args[2]->IsNumber() ) {
            exp = time(0) + (args[2]->IntegerValue());
        }

        if( args[2]->IsString() ) {
            String::Utf8Value keyg(args[2]);
            strcpy(gkey,*keyg);
        }
    }
    if( args.Length() > 3) {
        if( args[3]->IsNumber() ) {
            exp = time(0) + (args[3]->IntegerValue());
        }
    }

    if( !strlen(gkey)){
        rc = memcached_add(memc,*key, strlen(*key), val, val_l,exp, 0);
    }else{
        rc = memcached_add_by_key(memc, gkey, strlen(gkey),*key, strlen(*key), val, val_l,exp, 0);
    }

    if( rc) {
        fprintf(stderr,"add ERROR :: %d %s \n",rc,memcached_strerror(memc,rc));
        return handle_scope.Close(Boolean::New(false));
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>mc_app(const Arguments &args)//key,val,gkey?|exp?,exp?
{
    HandleScope handle_scope;

    if( args.Length() < 2) return Boolean::New(false);
    String::Utf8Value key(args[0]);
    String::Utf8Value vl(args[1]);
    char *val = *vl;
    size_t val_l = strlen(val);

    if( args[1]->IsObject() ) {
        Local<Object> obj = args[1]->ToObject();
        Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
        hBuf* o = (hBuf*)field->Value();
        int32_t id = obj->GetInternalField(1)->Int32Value();
        if( id != Buf_ID) return Boolean::New(false);

        val = o->p + o->beg;
        val_l = o->end - o->beg;
    }

    char gkey[300] = "";
    time_t exp = 0;

    memcached_st * memc = (memcached_st *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

    memcached_return_t rc = MEMCACHED_SUCCESS;
    size_t value_length;
    char *zn = 0;

    if( args.Length() > 2) {
        if( args[2]->IsNumber() ) {
            exp = time(0) + (args[2]->IntegerValue());
        }

        if( args[2]->IsString() ) {
            String::Utf8Value keyg(args[2]);
            strcpy(gkey,*keyg);
        }
    }
    if( args.Length() > 3) {
        if( args[3]->IsNumber() ) {
            exp = time(0) + (args[3]->IntegerValue());
        }
    }

    if( !strlen(gkey)){
        rc = memcached_append(memc,*key, strlen(*key), val, val_l,exp, 0);
    }else{
        rc = memcached_append_by_key(memc, gkey, strlen(gkey),*key, strlen(*key), val, val_l,exp, 0);
    }

    if( rc) {
        fprintf(stderr,"app ERROR :: %d %s \n",rc,memcached_strerror(memc,rc));
        return handle_scope.Close(Boolean::New(false));
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>mc_pre(const Arguments &args)//key,val,gkey?|exp?,exp?
{
    HandleScope handle_scope;

    if( args.Length() < 2) return Boolean::New(false);
    String::Utf8Value key(args[0]);
    String::Utf8Value vl(args[1]);
    char *val = *vl;
    size_t val_l = strlen(val);

    if( args[1]->IsObject() ) {
        Local<Object> obj = args[1]->ToObject();
        Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
        hBuf* o = (hBuf*)field->Value();
        int32_t id = obj->GetInternalField(1)->Int32Value();
        if( id != Buf_ID) return Boolean::New(false);

        val = o->p + o->beg;
        val_l = o->end - o->beg;
    }

    char gkey[300] = "";
    time_t exp = 0;

    memcached_st * memc = (memcached_st *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

    memcached_return_t rc = MEMCACHED_SUCCESS;
    size_t value_length;
    char *zn = 0;

    if( args.Length() > 2) {
        if( args[2]->IsNumber() ) {
            exp = time(0) + (args[2]->IntegerValue());
        }

        if( args[2]->IsString() ) {
            String::Utf8Value keyg(args[2]);
            strcpy(gkey,*keyg);
        }
    }
    if( args.Length() > 3) {
        if( args[3]->IsNumber() ) {
            exp = time(0) + (args[3]->IntegerValue());
        }
    }

    if( !strlen(gkey)){
        rc = memcached_prepend(memc,*key, strlen(*key), val, val_l,exp, 0);
    }else{
        rc = memcached_prepend_by_key(memc, gkey, strlen(gkey),*key, strlen(*key), val, val_l,exp, 0);
    }

    if( rc) {
        fprintf(stderr,"pre ERROR :: %d %s \n",rc,memcached_strerror(memc,rc));
        return handle_scope.Close(Boolean::New(false));
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>mc_replace(const Arguments &args)//key,val,gkey?|exp?,exp?
{
    HandleScope handle_scope;

    if( args.Length() < 2) return Boolean::New(false);
    String::Utf8Value key(args[0]);
    String::Utf8Value vl(args[1]);
    char *val = *vl;
    size_t val_l = strlen(val);

    if( args[1]->IsObject() ) {
        Local<Object> obj = args[1]->ToObject();
        Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
        hBuf* o = (hBuf*)field->Value();
        int32_t id = obj->GetInternalField(1)->Int32Value();
        if( id != Buf_ID) return Boolean::New(false);

        val = o->p + o->beg;
        val_l = o->end - o->beg;
    }

    char gkey[300] = "";
    time_t exp = 0;

    memcached_st * memc = (memcached_st *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

    memcached_return_t rc = MEMCACHED_SUCCESS;
    size_t value_length;
    char *zn = 0;

    if( args.Length() > 2) {
        if( args[2]->IsNumber() ) {
            exp = time(0) + (args[2]->IntegerValue());
        }

        if( args[2]->IsString() ) {
            String::Utf8Value keyg(args[2]);
            strcpy(gkey,*keyg);
        }
    }
    if( args.Length() > 3) {
        if( args[3]->IsNumber() ) {
            exp = time(0) + (args[3]->IntegerValue());
        }
    }

    if( !strlen(gkey)){
        rc = memcached_replace(memc,*key, strlen(*key), val, val_l,exp, 0);
    }else{
        rc = memcached_replace_by_key(memc, gkey, strlen(gkey),*key, strlen(*key), val, val_l,exp, 0);
    }

    if( rc) {
        fprintf(stderr,"repl ERROR :: %d %s \n",rc,memcached_strerror(memc,rc));
        return handle_scope.Close(Boolean::New(false));
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>mc_del(const Arguments &args)//key,gkey?|exp?,exp?
{
    HandleScope handle_scope;

    if( args.Length() < 1) return Boolean::New(false);
    String::Utf8Value key(args[0]);

    char gkey[300] = "";
    time_t exp = 0;

    memcached_st * memc = (memcached_st *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

    memcached_return_t rc = MEMCACHED_SUCCESS;

    if( args.Length() > 1) {
        if( args[1]->IsNumber() ) {
            exp = time(0) + (args[1]->IntegerValue());
        }

        if( args[1]->IsString() ) {
            String::Utf8Value keyg(args[1]);
            strcpy(gkey,*keyg);
        }
    }
    if( args.Length() > 2) {
        if( args[2]->IsNumber() ) {
            exp = time(0) + (args[2]->IntegerValue());
        }
    }

    if( !strlen(gkey)){
        rc = memcached_delete(memc,*key, strlen(*key),exp);
    }else{
        rc = memcached_delete_by_key(memc, gkey, strlen(gkey),*key, strlen(*key),exp);
    }

    if( rc) {
        fprintf(stderr,"del ERROR :: %d %s \n",rc,memcached_strerror(memc,rc));
        return handle_scope.Close(Boolean::New(false));
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>mc_inc(const Arguments &args)// key,(init|gkey)?,init?
{
    HandleScope handle_scope;

    if( args.Length() < 1) return Boolean::New(false);
    String::Utf8Value key(args[0]);
    char gkey[300] = "";

    memcached_st * memc = (memcached_st *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

    uint64_t init = 0;
    uint64_t value = 0;
    memcached_return_t rc = MEMCACHED_SUCCESS;

    if( args.Length() >= 2) {
        if( args[1]->IsNumber() ) {
            init = (args[1]->IntegerValue());
        }
        if( args[1]->IsString() ) {
            String::Utf8Value keyg(args[1]);
            strcpy(gkey,*keyg);
        }
    }
    if( args.Length() == 3) {
        if( args[2]->IsNumber() ) {
            init = (args[2]->IntegerValue());
        }
    }
    if( !strlen(gkey)){
        rc = memcached_increment_with_initial(memc, *key, strlen(*key),(uint64_t)1,(uint64_t) init, (time_t)0, &value);
//        fprintf(stderr,"memcached_increment_with_initial :: %s %s\n",memcached_strerror(memc,rc),gkey);
    }else{
        rc = memcached_increment_with_initial_by_key(memc, gkey, strlen(gkey), *key, strlen(*key),(uint64_t)1,(uint64_t) init, (time_t)0, &value);
//        fprintf(stderr,"memcached_increment_with_initial_by_key :: %s %s\n",memcached_strerror(memc,rc),gkey);
    }

    if( rc) {
        fprintf(stderr,"inc ERROR :: %d %s \n",rc,memcached_strerror(memc,rc));
    }

    return handle_scope.Close(Number::New(value));
}

static v8::Handle<v8::Value>mc_dec(const Arguments &args)// key,(init|gkey)?,init?
{
    HandleScope handle_scope;

    if( args.Length() < 1) return Boolean::New(false);
    String::Utf8Value key(args[0]);
    char gkey[300] = "";

    memcached_st * memc = (memcached_st *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

    uint64_t init = 0;
    uint64_t value = 0;
    memcached_return_t rc;

    if( args.Length() >= 2) {
        if( args[1]->IsNumber() ) {
            init = (args[1]->IntegerValue());
        }
        if( args[1]->IsString() ) {
            String::Utf8Value keyg(args[1]);
            strcpy(gkey,*keyg);
        }
    }
    if( args.Length() == 3) {
        if( args[2]->IsNumber() ) {
            init = (args[2]->IntegerValue());
        }
    }
    if( !strlen(gkey)){
        rc = memcached_decrement_with_initial(memc, *key, strlen(*key),(uint64_t)1,(uint64_t) init, (time_t)0, &value);
    }else{
        rc = memcached_decrement_with_initial_by_key(memc, gkey, strlen(gkey), *key, strlen(*key),(uint64_t)1,(uint64_t) init, (time_t)0, &value);
    }

    if( rc) {
        fprintf(stderr,"dec ERROR :: %d %s \n",rc,memcached_strerror(memc,rc));
    }

    return handle_scope.Close(Number::New(value));
}

static v8::Handle<v8::Value>mc_buf(const Arguments &args)// YN?
{
    HandleScope handle_scope;

    int yn = 1;
    if( args.Length() > 0) {
        yn = (int)(args[0]->IntegerValue());
    }
    memcached_st * memc = (memcached_st *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS, yn);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>mc_flush(const Arguments &args)
{
    HandleScope handle_scope;

    memcached_st * memc = (memcached_st *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
    memcached_flush_buffers(memc);

//    char b[] = "Странности только начинаются";
//    (*bb_buf)(b,strlen(b));

    return handle_scope.Close(Boolean::New(true));
}

//memcached_string_st
static v8::Persistent<v8::Object> make_connect()
{
    Handle<v8::ObjectTemplate> my_template = (Handle<v8::ObjectTemplate>)v8::ObjectTemplate::New();
    my_template->SetInternalFieldCount(2);
    my_template->Set(v8::String::NewSymbol("get"),v8::FunctionTemplate::New(mc_get)->GetFunction());
    my_template->Set(v8::String::NewSymbol("mget"),v8::FunctionTemplate::New(mc_mget)->GetFunction());
    my_template->Set(v8::String::NewSymbol("fetch"),v8::FunctionTemplate::New(mc_fetch)->GetFunction());
    my_template->Set(v8::String::NewSymbol("fetchb"),v8::FunctionTemplate::New(mc_fetchb)->GetFunction());
    my_template->Set(v8::String::NewSymbol("getb"),v8::FunctionTemplate::New(mc_getb)->GetFunction());
    my_template->Set(v8::String::NewSymbol("exist"),v8::FunctionTemplate::New(mc_exist)->GetFunction());

    my_template->Set(v8::String::NewSymbol("set"),v8::FunctionTemplate::New(mc_set)->GetFunction());
    my_template->Set(v8::String::NewSymbol("add"),v8::FunctionTemplate::New(mc_add)->GetFunction());
    my_template->Set(v8::String::NewSymbol("del"),v8::FunctionTemplate::New(mc_del)->GetFunction());
    my_template->Set(v8::String::NewSymbol("replace"),v8::FunctionTemplate::New(mc_replace)->GetFunction());
    my_template->Set(v8::String::NewSymbol("app"),v8::FunctionTemplate::New(mc_app)->GetFunction());
    my_template->Set(v8::String::NewSymbol("pre"),v8::FunctionTemplate::New(mc_pre)->GetFunction());
    my_template->Set(v8::String::NewSymbol("cas"),v8::FunctionTemplate::New(mc_cas)->GetFunction());

    my_template->Set(v8::String::NewSymbol("inc"),v8::FunctionTemplate::New(mc_inc)->GetFunction());
    my_template->Set(v8::String::NewSymbol("dec"),v8::FunctionTemplate::New(mc_dec)->GetFunction());

    my_template->Set(v8::String::NewSymbol("buf"),v8::FunctionTemplate::New(mc_buf)->GetFunction());
    my_template->Set(v8::String::NewSymbol("flush"),v8::FunctionTemplate::New(mc_flush)->GetFunction());


    Persistent<Object> obj = Persistent<Object>::New(my_template->NewInstance());

    my_template.Clear();

    return obj;
}

static v8::Handle<v8::Value>mccon(const Arguments &args)
{
    HandleScope handle_scope;
    int id = MEMCACHED_ID;
    char buf[5000] = "--SERVER=127.0.0.1";

//    if( args.Length() < 1) return Boolean::New(false);

    if( args.Length() > 0) {
        String::Utf8Value str(args[0]);
        strcpy(buf,*str);
    }

    //fprintf(stderr,"buf %s \n",buf);

    Persistent<Object> obj = make_connect();
    memcached_st *memc= memcached(buf, strlen(buf));

    if( !memc) return Boolean::New(false);

    if( args.Length() <= 1) {
        memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, true);
    }

    //--

    obj.MakeWeak(memc, objectWeakCallback);

    obj->SetInternalField(0, External::New(memc));
    obj->SetInternalField(1, Int32::New(id));

    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>pool_pop(const Arguments &args)
{
    HandleScope handle_scope;
    int id = MEMCACHED_ID;

    memcached_pool_st * ro = (memcached_pool_st *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

    memcached_return_t rc;
    memcached_st *memc= memcached_pool_pop(ro, false, &rc);
    if( MEMCACHED_SUCCESS != rc) {
        return Boolean::New(false);
    }
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, true);

    Persistent<Object> obj = make_connect();
    obj->SetInternalField(0, External::New(memc));
    obj->SetInternalField(1, Int32::New(id));

    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>pool_push(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return Boolean::New(false);

    Local<Object> obj = args[0]->ToObject();
    Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
    void* o = field->Value();
    int32_t id = obj->GetInternalField(1)->Int32Value();
    if( id != MEMCACHED_ID) return Boolean::New(false);

    memcached_st *memc = (memcached_st *)o;

    memcached_pool_st * ro = (memcached_pool_st *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

    memcached_return_t rc = memcached_pool_push(ro, memc);
    if( MEMCACHED_SUCCESS != rc) {
        return Boolean::New(false);
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>mccon_pool(const Arguments &args)
{
    HandleScope handle_scope;

    int id = MEMCACHED_POOL_ID;

    if( args.Length() < 1) return Boolean::New(false);
    String::Utf8Value str(args[0]);

    Handle<v8::ObjectTemplate> my_template = (Handle<v8::ObjectTemplate>)v8::ObjectTemplate::New();
    my_template->SetInternalFieldCount(2);
    my_template->Set(v8::String::NewSymbol("pop"),v8::FunctionTemplate::New(pool_pop)->GetFunction());
    my_template->Set(v8::String::NewSymbol("push"),v8::FunctionTemplate::New(pool_push)->GetFunction());

    Persistent<Object> obj = Persistent<Object>::New(my_template->NewInstance());

    my_template.Clear();

    memcached_pool_st *pool= memcached_pool(*str, strlen(*str));

    //--
    obj.MakeWeak(pool, objectWeakCallback);

    obj->SetInternalField(0, External::New(pool));
    obj->SetInternalField(1, Int32::New(id));

    return handle_scope.Close(obj);
}

//v8::Handle<v8::Value> init(v8::Persistent<v8::Context> ctx,v8::Handle<v8::Value>(*out_buf)(char *b,int sz))
void mc_init()
{
    bb_buf = out_buf;
    Handle<Object> global = context->Global();

//    char b[] = "MEMCACHED Plugin loaded!";
//    (*bb_buf)(b,strlen(b));

//    fprintf(stderr,"%s\n",b);fflush(stderr);

//    fprintf(stderr,"zz:: %d \n",zz);fflush(stderr);
//    fprintf(stderr,"context:: %ld \n",context);fflush(stderr);
//    fprintf(stderr,"context:: %ld '%s'\n",glb_arr,glb_arr[0]);fflush(stderr);
//    fprintf(stderr,"context:: %ld\n",glb_arr);fflush(stderr);

//    HandleScope handle_scope;

    Handle<Object> obj = Object::New();

    obj->Set(v8::String::NewSymbol("connect"),v8::FunctionTemplate::New(mccon)->GetFunction());
    obj->Set(v8::String::NewSymbol("pool"),v8::FunctionTemplate::New(mccon_pool)->GetFunction());

    global->Set(v8::String::NewSymbol("mcachjs"), obj);

//    return handle_scope.Close(obj);
}
/*
v8::Handle<v8::Value> init_hh(v8::Persistent<v8::Context> ctx)
{
    fprintf(stderr,"ldt init_hh::\n");fflush(stderr);

    HandleScope handle_scope;

    Handle<Object> obj = Object::New();

//    obj->Set(v8::String::NewSymbol("connect"),v8::FunctionTemplate::New(mccon)->GetFunction());
//    obj->Set(v8::String::NewSymbol("pool"),v8::FunctionTemplate::New(mccon_pool)->GetFunction());

    return handle_scope.Close(obj);
}
*/
