#include "qparsejs.h"
/*
static v8::Handle<v8::Value>q_dumpJS(const char *s, jsmntok_t *t, size_t count, int indent,int *jj)
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
*/
//static char b[10000];

static v8::Handle<v8::Value>q_dumpJS(char** s)
{
    HandleScope handle_scope;
    v8::Handle<v8::Value> o = (v8::Handle<v8::Value>)0;
    char * p = *s;
//    int state = 0;

    while( *p ) {
        if( isspace(*p) || *p == ','  || *p == ':') {++p; continue;}

        if( *p == '}' || *p == ']') {
            ++p;
            o = (v8::Handle<v8::Value>)0;
            break;
        }

        if( *p == '{' ) {
            ++p;
            Local<Object> ob = Object::New();
            v8::Handle<v8::Value> o1;
            v8::Handle<v8::Value> o2;

            while(1) {
                o1 = q_dumpJS(&p);
                if( o1 == ( v8::Handle<v8::Value> )0) break;

                o2 = q_dumpJS(&p);
                if( o2 == ( v8::Handle<v8::Value> )0) break;

                ob->Set(o1,o2);
            }

            o = ob;
            break;
        }

        if( *p == '[' ) {
            ++p;
            Local<Array> ob = Array::New();
            v8::Handle<v8::Value> oo;
            int i = 0;
            while( (oo = q_dumpJS(&p)) != (v8::Handle<v8::Value>)0){
                ob->Set(v8::Number::New(i),oo);
                ++i;
            }

            o = ob;
            break;
        }

        if( *p == '"' ) {// строка
            ++p;
            char *pp = p;
            while( pp = strchr(pp,'"')) {
                if(*(pp-1) != '\\') break;
                ++pp;
            }
            if( !pp) {
                o = (v8::Handle<v8::Value>)0;
                break;
            }
            o = v8::String::New( p, pp - p);
            p = pp+1;
            break;
        }

        //примитив

        char *pp = p;
        char stop[] = ",:{}[]\" \t\n\v\f\r";
        for(; !strchr(stop,*pp) ; ++pp) ;

        char c = *p;
        if( c == 'f' || c == 'F') {
            o = Boolean::New(false);
        }else
        if( c == 't' || c == 'T') {
            o = Boolean::New(true);
        }else
        if( c == 'n' || c == 'N') {
            o = v8::Null();
        }else {
            o = Number::New( atof( p));
        }

        p = pp;
        break;
    }

    *s = p;

    return handle_scope.Close(o);
}

v8::Handle<v8::Value>q_parseJS(char* s)
{
    HandleScope handle_scope;
    char * p = s;

    v8::Handle<v8::Value>o = q_dumpJS(&p);
    if( o == ( v8::Handle<v8::Value> )0) return handle_scope.Close(Boolean::New(false));

    return handle_scope.Close(o);
}
