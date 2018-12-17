#ifndef MYJS_H
#define MYJS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <QtCore>

#include <v8.h>
using namespace v8;

#ifdef _WIN32
#define MY_EXPORT __declspec(dllexport)
#else
#define MY_EXPORT
#endif

extern "C" {
  MY_EXPORT v8::Handle<v8::Value>  init(v8::Persistent<v8::Context> ctx);
}

#endif
