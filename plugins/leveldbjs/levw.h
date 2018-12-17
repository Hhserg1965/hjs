#ifndef levJS_H
#define levJS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cassert>
#include "leveldb/db.h"
#include "leveldb/cache.h"
#include "leveldb/filter_policy.h"
#include "leveldb/write_batch.h"
#include "leveldb/options.h"

#include <v8.h>
using namespace v8;

#ifdef _WIN32
#define MY_EXPORT __declspec(dllexport)
#else
#define MY_EXPORT
#endif

class levC
{
public:

    leveldb::DB* db;
    leveldb::Cache* block_cache;
    const leveldb::FilterPolicy* filter_policy;

    levC(){
        db = 0;
        block_cache = 0;
        filter_policy = 0;
    }
};

class BlevC
{
public:

    leveldb::DB* db;
    leveldb::WriteBatch *batch;

    BlevC(){
        db = 0;
        batch = 0;
    }
};

class IlevC
{
public:

    leveldb::DB* db;
    leveldb::Iterator* it;
    const leveldb::Snapshot* ss;

    IlevC(){
        db = 0;
        it = 0;
        ss = 0;
    }
};

extern "C" {
    MY_EXPORT v8::Handle<v8::Value>  init(v8::Persistent<v8::Context> ctx,v8::Handle<v8::Value>(*out_buf)(char *b,int sz));
//    MY_EXPORT v8::Handle<v8::Value> init_hh(v8::Persistent<v8::Context> ctx);
}

#endif
