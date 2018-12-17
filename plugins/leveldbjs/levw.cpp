#include "levw.h"

//extern v8::Persistent<v8::Context> context;

//Persistent<v8::ObjectTemplate> cvw_template;
//Persistent<v8::ObjectTemplate> mat_template;
//static v8::Handle<v8::ObjectTemplate>mat_t();

//Persistent<v8::ObjectTemplate> bdb_template;

v8::Handle<v8::Value>(*bb_buf)(char *b,int sz);

const char *db_home_dir = "./leveldb/";

enum{
    LEV_ID=22400,BLEV_ID,ILEV_ID
};

static void objectWeakCallback(v8::Persistent<v8::Value> obj, void* parameter)
{
    v8::Persistent<v8::Object> extObj = v8::Persistent<v8::Object>::Cast(obj);
    void * par = (void *)extObj->GetAlignedPointerFromInternalField(0);

    v8::Handle<v8::External> extObjID = v8::Handle<v8::External>::Cast(extObj->GetInternalField(1));

    switch( extObjID->Int32Value())
    {


    }

    obj.ClearWeak();
//    obj.Dispose();
}

static v8::Handle<v8::Value>closeLev(const Arguments &args)
{
    HandleScope handle_scope;

    levC * c = (levC *)args.This()->GetAlignedPointerFromInternalField(0);

    if( c) {
        delete  c->db;

        if( c->block_cache) delete c->block_cache;
        if( c->filter_policy) delete c->filter_policy;

        delete c;

        args.This()->SetAlignedPointerInInternalField(0, 0);
    }

    return handle_scope.Close(Boolean::New(true));
}
static v8::Handle<v8::Value>bcloseLev(const Arguments &args)
{
    HandleScope handle_scope;

    BlevC * c = (BlevC *)args.This()->GetAlignedPointerFromInternalField(0);

    if( c) {
        if( c->batch) delete c->batch;
        delete c;

        args.This()->SetAlignedPointerInInternalField(0, 0);
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>getLev(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    levC * c = (levC *)args.This()->GetAlignedPointerFromInternalField(0);
    String::Utf8Value k(args[0]);

    std::string value;
    leveldb::Status s = c->db->Get(leveldb::ReadOptions(), *k, &value);

    if( !s.ok()) {
        return handle_scope.Close(Boolean::New(false));
    }

    if( args.Length() > 1) {
        v8::Handle<v8::Value> o = (*bb_buf)((char*)value.c_str(),value.length());
        value.clear();
        return handle_scope.Close( o);
    }

    v8::Handle<v8::Value> o = String::New((const char*)value.c_str());
    value.clear();
    return handle_scope.Close( o);
}

static v8::Handle<v8::Value>putLev(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 2) return handle_scope.Close(Boolean::New(false));

    levC * c = (levC *)args.This()->GetAlignedPointerFromInternalField(0);

    String::Utf8Value k(args[0]);
    String::Utf8Value v(args[1]);

    leveldb::WriteOptions write_options;
    write_options.sync = false;
    if( args.Length() > 2) write_options.sync = true;

    leveldb::Status s = c->db->Put(write_options, *k, *v);

    if( !s.ok()) {
        return handle_scope.Close(Boolean::New(false));
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>bputLev(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 2) return handle_scope.Close(Boolean::New(false));

    BlevC * c = (BlevC *)args.This()->GetAlignedPointerFromInternalField(0);

    String::Utf8Value k(args[0]);
    String::Utf8Value v(args[1]);

    c->batch->Put(*k,*v);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>delLev(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    levC * c = (levC *)args.This()->GetAlignedPointerFromInternalField(0);

    String::Utf8Value k(args[0]);

    leveldb::WriteOptions write_options;
    write_options.sync = false;
    if( args.Length() > 1) write_options.sync = true;

    leveldb::Status s = c->db->Delete(write_options, *k);

    if( !s.ok()) {
        return handle_scope.Close(Boolean::New(false));
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>syncLev(const Arguments &args)
{
    HandleScope handle_scope;

    levC * c = (levC *)args.This()->GetAlignedPointerFromInternalField(0);
    leveldb::WriteOptions write_options;
    write_options.sync = true;

    leveldb::Status s = c->db->Delete(write_options,"---AutomaticSync---");

    if( !s.ok()) {
        return handle_scope.Close(Boolean::New(false));
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>bdelLev(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    BlevC * c = (BlevC *)args.This()->GetAlignedPointerFromInternalField(0);

    String::Utf8Value k(args[0]);

    c->batch->Delete(*k);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>bwriteLev(const Arguments &args)
{
    HandleScope handle_scope;

    BlevC * c = (BlevC *)args.This()->GetAlignedPointerFromInternalField(0);
    if( !c) return handle_scope.Close(Boolean::New(false));
    if( !c->db || !c->batch) return handle_scope.Close(Boolean::New(false));

    leveldb::WriteOptions write_options;
    if( args.Length() > 0)
        write_options.sync = true;

    leveldb::Status s = c->db->Write(write_options, c->batch);

    delete c->batch;
    delete c;
    args.This()->SetAlignedPointerInInternalField(0, 0);

    if( !s.ok()) {
        return handle_scope.Close(Boolean::New(false));
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>batchLev(const Arguments &args)
{
    HandleScope handle_scope;

    levC * c = (levC *)args.This()->GetAlignedPointerFromInternalField(0);
    if( !c) return handle_scope.Close(Boolean::New(false));

    BlevC * b = new BlevC();
    b->db = c->db;
    b->batch = new leveldb::WriteBatch();

    Handle<v8::ObjectTemplate> lev_template = (Handle<v8::ObjectTemplate>)v8::ObjectTemplate::New();
    lev_template->SetInternalFieldCount(2);

    lev_template->Set(v8::String::NewSymbol("close"),v8::FunctionTemplate::New(bcloseLev)->GetFunction());
    lev_template->Set(v8::String::NewSymbol("rollback"),v8::FunctionTemplate::New(bcloseLev)->GetFunction());
    lev_template->Set(v8::String::NewSymbol("rb"),v8::FunctionTemplate::New(bcloseLev)->GetFunction());

//    lev_template->Set(v8::String::NewSymbol("get"),v8::FunctionTemplate::New(bgetLev)->GetFunction());

    lev_template->Set(v8::String::NewSymbol("put"),v8::FunctionTemplate::New(bputLev)->GetFunction());
    lev_template->Set(v8::String::NewSymbol("set"),v8::FunctionTemplate::New(bputLev)->GetFunction());

    lev_template->Set(v8::String::NewSymbol("del"),v8::FunctionTemplate::New(bdelLev)->GetFunction());

    lev_template->Set(v8::String::NewSymbol("write"),v8::FunctionTemplate::New(bwriteLev)->GetFunction());
    lev_template->Set(v8::String::NewSymbol("commit"),v8::FunctionTemplate::New(bwriteLev)->GetFunction());
    lev_template->Set(v8::String::NewSymbol("cm"),v8::FunctionTemplate::New(bwriteLev)->GetFunction());

    //--

    Persistent<Object> obj = Persistent<Object>::New(lev_template->NewInstance());

    lev_template.Clear();

    obj->SetAlignedPointerInInternalField(0, b);
    obj->SetInternalField(1, Int32::New(BLEV_ID));

    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>icloseLev(const Arguments &args)
{
    HandleScope handle_scope;

    IlevC * c = (IlevC *)args.This()->GetAlignedPointerFromInternalField(0);

    if( c) {

        if( c->it) delete c->it;
        if( c->ss) c->db->ReleaseSnapshot( c->ss);

        delete c;

        args.This()->SetAlignedPointerInInternalField(0, 0);
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>nextLev(const Arguments &args)
{
    HandleScope handle_scope;

    IlevC * c = (IlevC *)args.This()->GetAlignedPointerFromInternalField(0);

    if(c->it->Valid()){
        c->it->Next();
    }else{
        return handle_scope.Close(Boolean::New(false));
    }

    return handle_scope.Close(Boolean::New(c->it->Valid()));
}

static v8::Handle<v8::Value>prevLev(const Arguments &args)
{
    HandleScope handle_scope;

    IlevC * c = (IlevC *)args.This()->GetAlignedPointerFromInternalField(0);

    if(c->it->Valid()){
        c->it->Prev();
    }else{
        return handle_scope.Close(Boolean::New(false));
    }

    return handle_scope.Close(Boolean::New(c->it->Valid()));
}

static v8::Handle<v8::Value>validLev(const Arguments &args)
{
    HandleScope handle_scope;

    IlevC * c = (IlevC *)args.This()->GetAlignedPointerFromInternalField(0);

    return handle_scope.Close(Boolean::New(c->it->Valid()));
}

static v8::Handle<v8::Value>keyLev(const Arguments &args)
{
    HandleScope handle_scope;

    IlevC * c = (IlevC *)args.This()->GetAlignedPointerFromInternalField(0);
    leveldb::Slice s = c->it->key();
    return handle_scope.Close(String::New(s.data(),s.size()));
}

static v8::Handle<v8::Value>keyLevD(const Arguments &args)
{
    HandleScope handle_scope;

    IlevC * c = (IlevC *)args.This()->GetAlignedPointerFromInternalField(0);
    leveldb::Slice s = c->it->key();
    return handle_scope.Close(Number::New(*(long*)s.data()));
}

static v8::Handle<v8::Value>valLev(const Arguments &args)
{
    HandleScope handle_scope;

    IlevC * c = (IlevC *)args.This()->GetAlignedPointerFromInternalField(0);
    leveldb::Slice s = c->it->value();
    if( args.Length() > 0) return handle_scope.Close((*bb_buf)((char*)s.data(),s.size()));

    return handle_scope.Close(String::New(s.data(),s.size()));
}

static v8::Handle<v8::Value>findLev(const Arguments &args)
{
    HandleScope handle_scope;

    levC * c = (levC *)args.This()->GetAlignedPointerFromInternalField(0);
    if( !c) return handle_scope.Close(Boolean::New(false));

    IlevC * it = new IlevC();
    it->db = c->db;

    leveldb::ReadOptions options;
    if( args.Length() > 1) options.snapshot = c->db->GetSnapshot();
    it->ss = options.snapshot;

    leveldb::Iterator* iter = c->db->NewIterator(options);
    it->it = iter;

    if( args.Length() == 0) {
        iter->SeekToFirst();
    }else {
        if( args[0]->IsBoolean()) {
            if( args[0]->BooleanValue()){
                iter->SeekToFirst();
            }else {
                iter->SeekToLast();
            }
        }else {
            String::Utf8Value k(args[0]);
            iter->Seek(*k);
        }
    }

    Handle<v8::ObjectTemplate> lev_template = (Handle<v8::ObjectTemplate>)v8::ObjectTemplate::New();
    lev_template->SetInternalFieldCount(2);

    lev_template->Set(v8::String::NewSymbol("close"),v8::FunctionTemplate::New(icloseLev)->GetFunction());
    lev_template->Set(v8::String::NewSymbol("end"),v8::FunctionTemplate::New(icloseLev)->GetFunction());
    lev_template->Set(v8::String::NewSymbol("found"),v8::FunctionTemplate::New(icloseLev)->GetFunction());
    lev_template->Set(v8::String::NewSymbol("clr"),v8::FunctionTemplate::New(icloseLev)->GetFunction());

    lev_template->Set(v8::String::NewSymbol("next"),v8::FunctionTemplate::New(nextLev)->GetFunction());
    lev_template->Set(v8::String::NewSymbol("prev"),v8::FunctionTemplate::New(prevLev)->GetFunction());
    lev_template->Set(v8::String::NewSymbol("valid"),v8::FunctionTemplate::New(validLev)->GetFunction());

    lev_template->Set(v8::String::NewSymbol("k"),v8::FunctionTemplate::New(keyLev)->GetFunction());
    lev_template->Set(v8::String::NewSymbol("kd"),v8::FunctionTemplate::New(keyLevD)->GetFunction());
    lev_template->Set(v8::String::NewSymbol("key"),v8::FunctionTemplate::New(keyLev)->GetFunction());

    lev_template->Set(v8::String::NewSymbol("v"),v8::FunctionTemplate::New(valLev)->GetFunction());
    lev_template->Set(v8::String::NewSymbol("val"),v8::FunctionTemplate::New(valLev)->GetFunction());

    //--

    Persistent<Object> obj = Persistent<Object>::New(lev_template->NewInstance());

    lev_template.Clear();

    obj->SetAlignedPointerInInternalField(0, it);
    obj->SetInternalField(1, Int32::New(ILEV_ID));

    return handle_scope.Close(obj);
}

//#define LEV_BLK_SZ 4096
#define LEV_BLK_SZ 4096*2

static v8::Handle<v8::Value>openLev(const Arguments &args)
{
    HandleScope handle_scope;
    char b[2000];
    strcpy(b,db_home_dir);
    int c_sz = 50;

    int id = LEV_ID;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));
    String::Utf8Value   fn(args[0]);

    if( args.Length() > 1) {
        String::Utf8Value   dir(args[1]);
        strcpy(b,*dir);
    }

    int pr = 0755;
#ifdef _WIN32
    _mkdir(b);
#else
    mkdir(b,pr);
#endif

    if( args.Length() > 2) c_sz =  args[2]->Int32Value();

    leveldb::DB* db;
    leveldb::Options options;
    options.create_if_missing = true;
    options.block_cache = leveldb::NewLRUCache(c_sz * 1048576);
    options.filter_policy = leveldb::NewBloomFilterPolicy(10);
    options.block_size = LEV_BLK_SZ;

//    options.compression = leveldb::kNoCompression;
    options.compression = leveldb::kSnappyCompression;

//    options.paranoid_checks = true;

    strcat(b,*fn);
    leveldb::Status status = leveldb::DB::Open(options, b, &db);
    if( !status.ok()) {
        fprintf(stderr, "Error OpenLevDb: %s\n", status.ToString().c_str());fflush(stderr);

        delete options.block_cache;
        delete options.filter_policy;

        return handle_scope.Close(Boolean::New(false));
    }

    levC * lo = new levC();

    lo->db = db;
    lo->block_cache = options.block_cache;
    lo->filter_policy = options.filter_policy;

    Handle<v8::ObjectTemplate> lev_template = (Handle<v8::ObjectTemplate>)v8::ObjectTemplate::New();
    lev_template->SetInternalFieldCount(2);

    lev_template->Set(v8::String::NewSymbol("close"),v8::FunctionTemplate::New(closeLev)->GetFunction());
    lev_template->Set(v8::String::NewSymbol("get"),v8::FunctionTemplate::New(getLev)->GetFunction());
    lev_template->Set(v8::String::NewSymbol("put"),v8::FunctionTemplate::New(putLev)->GetFunction());
    lev_template->Set(v8::String::NewSymbol("set"),v8::FunctionTemplate::New(putLev)->GetFunction());
    lev_template->Set(v8::String::NewSymbol("del"),v8::FunctionTemplate::New(delLev)->GetFunction());
    lev_template->Set(v8::String::NewSymbol("sync"),v8::FunctionTemplate::New(syncLev)->GetFunction());

    lev_template->Set(v8::String::NewSymbol("batch"),v8::FunctionTemplate::New(batchLev)->GetFunction());
    lev_template->Set(v8::String::NewSymbol("find"),v8::FunctionTemplate::New(findLev)->GetFunction());

    //--

    Persistent<Object> obj = Persistent<Object>::New(lev_template->NewInstance());

    lev_template.Clear();

    obj->SetAlignedPointerInInternalField(0, lo);
    obj->SetInternalField(1, Int32::New(id));
//    obj->SetAlignedPointerInInternalField(2, 0);

    return handle_scope.Close(obj);
}

//v8::Handle<v8::Value> init(v8::Persistent<v8::Context> ctx)
//{

//typedef v8::Handle<v8::Value> (*MyPrototype)(v8::Persistent<v8::Context> ctx, v8::Handle<v8::Value>(*out_buf)(char *b,int sz));

v8::Handle<v8::Value> init(v8::Persistent<v8::Context> ctx,v8::Handle<v8::Value>(*out_buf)(char *b,int sz))
{
    bb_buf = out_buf;

    HandleScope handle_scope;

//    envp = NULL;

    //--

    Handle<Object> obj = Object::New();

    obj->Set(v8::String::NewSymbol("open"),v8::FunctionTemplate::New(openLev)->GetFunction()); // name hash
    return handle_scope.Close(obj);
}

