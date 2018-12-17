#include "myjs.h"
#include <mysql/mysql.h>

extern v8::Persistent<v8::Context> context;

//Persistent<v8::ObjectTemplate> cvw_template;
//Persistent<v8::ObjectTemplate> mat_template;
//static v8::Handle<v8::ObjectTemplate>mat_t();


enum{
MYSQL_ID=22000
};

static void objectWeakCallback(v8::Persistent<v8::Value> obj, void* parameter)
{
    v8::Persistent<v8::Object> extObj = v8::Persistent<v8::Object>::Cast(obj);
    void * par = (void *)(Handle<External>::Cast(extObj->GetInternalField(0)))->Value();
    v8::Handle<v8::External> extObjID = v8::Handle<v8::External>::Cast(extObj->GetInternalField(1));

    switch( extObjID->Int32Value())
    {
        case MYSQL_ID:
        {

            MYSQL *o = (MYSQL *) par;

            if( o) {
//                fprintf(stderr,"MYSQL_ID delete %ld\n",o);fflush(stderr);
                mysql_close(o);
            }
            break;
        }
    }

    obj.ClearWeak();
    obj.Dispose();
    obj.Clear();
}

static v8::Handle<v8::Value>my_err(const Arguments &args)
{
    HandleScope handle_scope;

    MYSQL * ro = (MYSQL *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
    Local<String> ov = String::New((const char*)mysql_error(ro));

    return handle_scope.Close(ov);
}

static v8::Handle<v8::Value>my_end(const Arguments &args)
{
    HandleScope handle_scope;

    MYSQL * ro = (MYSQL *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
    if( ro) {
        mysql_close(ro);
    }

    return handle_scope.Close(Boolean::New(true));
}


static v8::Handle<v8::Value>my_rows(const Arguments &args)
{
    HandleScope handle_scope;

    MYSQL * ro = (MYSQL *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
    int cnt = mysql_affected_rows(ro);

    return handle_scope.Close(Number::New(cnt));
}


static v8::Handle<v8::Value>my_sql(const Arguments &args)
{
    HandleScope handle_scope;

//fprintf(stderr,"my_sql\n");fflush(stderr);


    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));
    String::Utf8Value sql(args[0]);
    QString sq(*sql);

//fprintf(stderr,"1\n");fflush(stderr);

    QStringList sq_l = sq.split(QRegExp("\\s+"));
//fprintf(stderr,"1.1\n");fflush(stderr);
    QString nm = sq_l.at(0).simplified().toLower();

//fprintf(stderr,"2\n");fflush(stderr);

//QTextCodec *codec = QTextCodec::codecForName("UTF-8");
//fprintf(stderr,"%s\n%s\n",*sql,codec->fromUnicode(nm).data());fflush(stderr);


    MYSQL * conn = (MYSQL *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();

    if( mysql_query(conn,*sql) != 0)  {
        return handle_scope.Close(Boolean::New(false));
    }

    if( nm == "insert" || nm == "update") {
        return handle_scope.Close(Number::New(mysql_affected_rows(conn)));
    }

    if( nm == "select") {
        MYSQL_RES *result = mysql_store_result(conn);

        unsigned int num_fields;
        unsigned int i;
        MYSQL_FIELD *fields;

        num_fields = mysql_num_fields(result);
        fields = mysql_fetch_fields(result);
        for(i = 0; i < num_fields; i++)
        {
//           printf("Field %u is %s\n", i, fields[i].name);
        }

        Handle<Array> ar = Array::New();

        MYSQL_ROW row;
        unsigned int j=0;

        while ((row = mysql_fetch_row(result)))
        {
           unsigned long *lengths;
           lengths = mysql_fetch_lengths(result);
//           Handle<Array> ar2 = Array::New();
           Handle<Object> obj = Object::New();

           for(i = 0; i < num_fields; i++)
           {
                if( !row[i]) {
                    obj->Set(v8::String::NewSymbol(fields[i].name),String::New("NULL"));
                    continue;
                }

                switch(fields[i].type) {

                case MYSQL_TYPE_DECIMAL:
                case MYSQL_TYPE_TINY:
                case MYSQL_TYPE_SHORT:
                case MYSQL_TYPE_LONG:

                case MYSQL_TYPE_LONGLONG:
                case MYSQL_TYPE_INT24:
                case MYSQL_TYPE_NEWDECIMAL:
                case MYSQL_TYPE_ENUM:
                case MYSQL_TYPE_SET:
//                    printf("int %s\n",fields[i].name);

                    obj->Set(v8::String::NewSymbol(fields[i].name),Number::New(atoll(row[i])));
                    break;

                case MYSQL_TYPE_FLOAT:
                case MYSQL_TYPE_DOUBLE:
//                    printf("double %s\n",fields[i].name);

                    obj->Set(v8::String::NewSymbol(fields[i].name),Number::New(atof(row[i])));
                    break;

                default:
//                    printf("string %s\n",fields[i].name);

                    obj->Set(v8::String::NewSymbol(fields[i].name),String::New(row[i]));
                    break;
                }

//               printf("[%.*s] ", (int) lengths[i],
//                      row[i] ? row[i] : "NULL");

//               printf("[%s] ",row[i] ? row[i] : "NULL");

//              ar2->Set(v8::Number::New(i),String::New(row[i] ? row[i] : "NULL"));
/*
enum enum_field_types { MYSQL_TYPE_DECIMAL, MYSQL_TYPE_TINY,
            MYSQL_TYPE_SHORT,  MYSQL_TYPE_LONG,
            MYSQL_TYPE_FLOAT,  MYSQL_TYPE_DOUBLE,
            MYSQL_TYPE_NULL,   MYSQL_TYPE_TIMESTAMP,
            MYSQL_TYPE_LONGLONG,MYSQL_TYPE_INT24,
            MYSQL_TYPE_DATE,   MYSQL_TYPE_TIME,
            MYSQL_TYPE_DATETIME, MYSQL_TYPE_YEAR,
            MYSQL_TYPE_NEWDATE, MYSQL_TYPE_VARCHAR,
            MYSQL_TYPE_BIT,
                        MYSQL_TYPE_NEWDECIMAL=246,
            MYSQL_TYPE_ENUM=247,
            MYSQL_TYPE_SET=248,
            MYSQL_TYPE_TINY_BLOB=249,
            MYSQL_TYPE_MEDIUM_BLOB=250,
            MYSQL_TYPE_LONG_BLOB=251,
            MYSQL_TYPE_BLOB=252,
            MYSQL_TYPE_VAR_STRING=253,
            MYSQL_TYPE_STRING=254,
            MYSQL_TYPE_GEOMETRY=255

};
*/


//                obj->Set(v8::String::NewSymbol(fields[i].name),String::New(row[i] ? row[i] : "NULL"));

           }
//           ar->Set(v8::Number::New(j),ar2);
           ar->Set(v8::Number::New(j),obj);
//           printf("\n");

           ++j;
        }

        mysql_free_result(result);
        return handle_scope.Close(ar);
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>mycon(const Arguments &args)
{
    HandleScope handle_scope;
    int id = MYSQL_ID;

    if( args.Length() < 4) return handle_scope.Close(Boolean::New(false));
    String::Utf8Value host(args[0]);//host,user,passwd,db,port?
    String::Utf8Value user(args[1]);
    String::Utf8Value passwd(args[2]);
    String::Utf8Value db(args[3]);

    int port = 3306;

    if( args.Length() > 4) {
        port = (int)(args[4]->NumberValue());
    }

    MYSQL 	*conn, *rconn;

    conn = mysql_init(NULL);

    if(conn == NULL)
    {
        mysql_close(conn);
        return handle_scope.Close(Boolean::New(false));
    }

    if( !( rconn = mysql_real_connect(conn,
                            *host,
                            *user,
                            *passwd,
                            *db,
                            port,
                            0,
                            0
                            )) )
    {
        fprintf(stderr,"ERROR: mysql_real_connect: %s\n",mysql_error(conn));fflush(stderr);
        mysql_close(conn);
        return handle_scope.Close(Boolean::New(false));
    }

    Handle<v8::ObjectTemplate> my_template = (Handle<v8::ObjectTemplate>)v8::ObjectTemplate::New();
    my_template->SetInternalFieldCount(2);
    my_template->Set(v8::String::NewSymbol("err"),v8::FunctionTemplate::New(my_err)->GetFunction());
    my_template->Set(v8::String::NewSymbol("rows"),v8::FunctionTemplate::New(my_rows)->GetFunction());
    my_template->Set(v8::String::NewSymbol("sql"),v8::FunctionTemplate::New(my_sql)->GetFunction());
    my_template->Set(v8::String::NewSymbol("end"),v8::FunctionTemplate::New(my_end)->GetFunction());

    Local<Object> obj = Local<Object>::New(my_template->NewInstance());

    //--
//    obj.MakeWeak(conn, objectWeakCallback);

    my_template.Clear();

    obj->SetInternalField(0, External::New(conn));
    obj->SetInternalField(1, Int32::New(id));

    return handle_scope.Close(obj);
}


static v8::Handle<v8::Value>myend(const Arguments &args)
{
    HandleScope handle_scope;

    mysql_library_end();

    return handle_scope.Close(Boolean::New(true));
}

v8::Handle<v8::Value> init(v8::Persistent<v8::Context> ctx)
{

    HandleScope handle_scope;

    mysql_library_init(0,NULL,NULL);

    Handle<Object> obj = Object::New();

//    obj->Set(v8::String::NewSymbol("x"),Int32::New(999));
//    obj->Set(v8::String::NewSymbol("y"),Int32::New(11));

    obj->Set(v8::String::NewSymbol("end"),v8::FunctionTemplate::New(myend)->GetFunction());
    obj->Set(v8::String::NewSymbol("con"),v8::FunctionTemplate::New(mycon)->GetFunction());//host,user,passwd,db,port?
    obj->Set(v8::String::NewSymbol("connect"),v8::FunctionTemplate::New(mycon)->GetFunction());

    return handle_scope.Close(obj);
}

