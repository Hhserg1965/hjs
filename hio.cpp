#include "hio.h"

#include "mainwindow.h"
using namespace v8;
QString rt(const char * s);
extern v8::Persistent<v8::Context> context;

v8::Handle<v8::Value>v8_js_print(const Arguments &args)
{
    QString result = "";
    for (int i = 0; i < args.Length(); ++i){
        String::Utf8Value ascii(args[i]);
        result += rt(*ascii);
    }

    QByteArray ba;
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    ba = codec->fromUnicode(result);

    fprintf(stderr,"%s",ba.data());fflush(stderr);

    return Boolean::New(true);
}

io::io()
{
    Handle<Object> global = context->Global();
    Local<Object> ob = Object::New();

    ob->Set(v8::String::NewSymbol("log"),v8::FunctionTemplate::New(v8_js_print)->GetFunction());

    global->Set(String::New("io"), ob);
}
