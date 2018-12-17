#include "vlcjs.h"

extern v8::Persistent<v8::Context> context;

//Persistent<v8::ObjectTemplate> cvw_template;
//Persistent<v8::ObjectTemplate> mat_template;
//static v8::Handle<v8::ObjectTemplate>mat_t();

char * media_name =  "Saver_Stream";
char p_pixels_null[10000000];

enum{
Player_ID=22000
};

static void objectWeakCallback(v8::Persistent<v8::Value> obj, void* parameter)
{
    v8::Persistent<v8::Object> extObj = v8::Persistent<v8::Object>::Cast(obj);
//    void * par = (void *)(Handle<External>::Cast(extObj->GetInternalField(0)))->Value();
    void * par = (void *)extObj->GetAlignedPointerFromInternalField(0);

    v8::Handle<v8::External> extObjID = v8::Handle<v8::External>::Cast(extObj->GetInternalField(1));

    switch( extObjID->Int32Value())
    {
/*
        case Mat_ID:
        {
            Mat *o = (Mat *) par;

            if( o) {
//                fprintf(stderr,"Mat_ID delete %ld\n",o);fflush(stderr);

                delete o;
            }

            break;
        }
        case VC_ID:
        {
            VideoCapture *o = (VideoCapture *) par;

            if( o) {
                o->release();
                delete o;
            }

            break;
        }
*/
    }

    obj.ClearWeak();
    obj.Dispose();
}

static v8::Handle<v8::Value>player_play(const Arguments &args)
{
    HandleScope handle_scope;

//    libvlc_media_player_t * player = (libvlc_media_player_t *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
    libvlc_media_player_t * player = (libvlc_media_player_t *)args.This()->GetAlignedPointerFromInternalField(2);

    if( player) libvlc_media_player_play(player);

//    libvlc_media_t * media = (libvlc_media_t *)(Handle<External>::Cast(args.This()->GetInternalField(1)))->Value();
    libvlc_media_t * media = (libvlc_media_t *)args.This()->GetAlignedPointerFromInternalField(1);

    libvlc_media_track_info_t *tracks=0;
    int cm = libvlc_media_get_tracks_info(media,&tracks);
    printf("libvlc_media_track_info_t cm %d %ld\n",cm,tracks);fflush(stdout);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>player_pause(const Arguments &args)
{
    HandleScope handle_scope;

//    libvlc_media_player_t * player = (libvlc_media_player_t *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
    libvlc_media_player_t * player = (libvlc_media_player_t *)args.This()->GetAlignedPointerFromInternalField(2);
    if( player) libvlc_media_player_pause(player);

    return handle_scope.Close(Boolean::New(true));
}
static v8::Handle<v8::Value>player_resume(const Arguments &args)
{
    HandleScope handle_scope;

//    libvlc_media_player_t * player = (libvlc_media_player_t *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
    libvlc_media_player_t * player = (libvlc_media_player_t *)args.This()->GetAlignedPointerFromInternalField(2);
    if( player) libvlc_media_player_set_pause(player,0);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>player_stop(const Arguments &args)
{
    HandleScope handle_scope;

//    libvlc_media_player_t * player = (libvlc_media_player_t *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
    libvlc_media_player_t * player = (libvlc_media_player_t *)args.This()->GetAlignedPointerFromInternalField(2);

    if( player) libvlc_media_player_stop(player);

    return handle_scope.Close(Boolean::New(true));
}
static v8::Handle<v8::Value>player_volume(const Arguments &args)
{
    HandleScope handle_scope;
    int vol = args[0]->Int32Value();

//    libvlc_media_player_t * player = (libvlc_media_player_t *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
    libvlc_media_player_t * player = (libvlc_media_player_t *)args.This()->GetAlignedPointerFromInternalField(2);

    if( player) libvlc_audio_set_volume(player,vol>10?vol:0);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>player_pos(const Arguments &args)
{
    HandleScope handle_scope;
    float cpos = 0;

//    libvlc_media_player_t * player = (libvlc_media_player_t *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
    libvlc_media_player_t * player = (libvlc_media_player_t *)args.This()->GetAlignedPointerFromInternalField(2);

    if( player) {
        if( args.Length() > 0) {
            double pos = args[0]->NumberValue();
            if( pos > 1) pos /= 100.;
            libvlc_media_player_set_position( player, (float) pos );
        }
        cpos = libvlc_media_player_get_position( player );
    }

    return handle_scope.Close(Number::New(cpos));
}

static v8::Handle<v8::Value>player_size(const Arguments &args)
{
    HandleScope handle_scope;
    float cpos = 0;

//    libvlc_media_player_t * player = (libvlc_media_player_t *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
    libvlc_media_player_t * player = (libvlc_media_player_t *)args.This()->GetAlignedPointerFromInternalField(2);

    if( player) {
        unsigned px;
        unsigned py;

        libvlc_video_get_size(	player,0,&px,&py);


        Handle<Object> obj = Object::New();

        obj->Set(v8::String::NewSymbol("x"),v8::Number::New(px));
        obj->Set(v8::String::NewSymbol("y"),v8::Number::New(py));

        return handle_scope.Close(obj);
    }

    return handle_scope.Close(Boolean::New(false));
}

static v8::Handle<v8::Value>player_del(const Arguments &args)
{
    HandleScope handle_scope;

//    libvlc_instance_t * instance = (libvlc_instance_t *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
//    libvlc_media_t * media = (libvlc_media_t *)(Handle<External>::Cast(args.This()->GetInternalField(1)))->Value();
//    libvlc_media_player_t * player = (libvlc_media_player_t *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();

    libvlc_instance_t * instance = (libvlc_instance_t *)args.This()->GetAlignedPointerFromInternalField(0);
    libvlc_media_t * media = (libvlc_media_t *)args.This()->GetAlignedPointerFromInternalField(1);
    libvlc_media_player_t * player = (libvlc_media_player_t *)args.This()->GetAlignedPointerFromInternalField(2);


    libvlc_media_player_stop(player);
    libvlc_media_player_release(player);
    libvlc_media_release(media);
    libvlc_release(instance);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::ObjectTemplate>player_t()
{
    HandleScope handle_scope;

    Handle<v8::ObjectTemplate> player_template = (Handle<v8::ObjectTemplate>)v8::ObjectTemplate::New();
    player_template->SetInternalFieldCount(3);

    player_template->Set(v8::String::NewSymbol("play"),v8::FunctionTemplate::New(player_play)->GetFunction());
    player_template->Set(v8::String::NewSymbol("pause"),v8::FunctionTemplate::New(player_pause)->GetFunction());
    player_template->Set(v8::String::NewSymbol("resume"),v8::FunctionTemplate::New(player_resume)->GetFunction());
    player_template->Set(v8::String::NewSymbol("stop"),v8::FunctionTemplate::New(player_stop)->GetFunction());
    player_template->Set(v8::String::NewSymbol("volume"),v8::FunctionTemplate::New(player_volume)->GetFunction());
    player_template->Set(v8::String::NewSymbol("pos"),v8::FunctionTemplate::New(player_pos)->GetFunction());
    player_template->Set(v8::String::NewSymbol("size"),v8::FunctionTemplate::New(player_size)->GetFunction());

    player_template->Set(v8::String::NewSymbol("del"),v8::FunctionTemplate::New(player_del)->GetFunction());

    player_template.Clear();

    return handle_scope.Close(player_template);
}

static v8::Handle<v8::Value>player(const Arguments &args)
{
    HandleScope handle_scope;
    int id = Player_ID;
    int i;
    libvlc_instance_t *m_vlcInstance = 0;
    libvlc_media_t *m_vlcMedia = 0;
    libvlc_media_player_t *m_vlcMediaplayer = 0;


    if( args.Length() < 2) return Boolean::New(false);
    String::Utf8Value ar(args[0]);
    String::Utf8Value fn(args[1]);

    int vlc_argc=0;
    char *vlc_argv[100];
    memset(vlc_argv,0,sizeof(vlc_argv));
    char *p = *ar;
    if( *p) {
        vlc_argv[vlc_argc++] = p;
        for(p++; *p; ++p) {
            if( *(p-1) == '\n') {
                vlc_argv[vlc_argc++] = p;
                *(p-1) = 0;
            }
        }
    }
///*
    printf("vlc_argc [%d]\n",vlc_argc);fflush(stdout);

    for(i=0; i < vlc_argc; ++i) {
        printf("arg [%s]\n",vlc_argv[i]);fflush(stdout);
    }
//*/
    printf("player %s\n%s\n",*fn,*ar);fflush(stdout);

    v8::Handle<v8::ObjectTemplate> player_template = player_t();
    Persistent<Object> obj = Persistent<Object>::New(player_template->NewInstance());

    if( args.Length() > 2) {
        Local<Object> obj = args[2]->ToObject();
//        Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
//        void* o = field->Value();
        QLabel * lb = (QLabel * )obj->GetAlignedPointerFromInternalField(0);

        m_vlcInstance = libvlc_new(vlc_argc, vlc_argv);
//        m_vlcMedia = libvlc_media_new_path(m_vlcInstance, *fn);
        m_vlcMedia = libvlc_media_new_location(m_vlcInstance, *fn);
        m_vlcMediaplayer = libvlc_media_player_new_from_media(m_vlcMedia);
//        libvlc_media_parse(m_vlcMedia);
//        libvlc_media_add_option(m_vlcMedia, "sout=\"#description\"");
        libvlc_media_player_set_xwindow(m_vlcMediaplayer,lb->winId());
        libvlc_media_player_play(m_vlcMediaplayer);

        printf("lb %d %d\n",lb->width(),lb->height());fflush(stdout);

        libvlc_media_track_info_t *tracks =0;
        int cm = libvlc_media_get_tracks_info(m_vlcMedia,&tracks);
        printf("libvlc_media_track_info_t cm %d %ld\n",cm,tracks);fflush(stdout);

        //--
/*
        bv = new BaseVideoClass();
    //    bv->setWitgetToPlay(ui->cw);


        bv->selectSource( QUrl("http://192.168.12.30:8001/1:0:1:69A0:1B:FFFF:1682FC2:0:0:0"));// url

    //    bv->setFileToPlay("1372144397.flv"); //file
    //    bv->readFromFile();// file

        bv->connectToSource();
        libvlc_media_player_set_xwindow(bv->m_vlcMediaplayer,ui->cw->winId());
        bv->play();
*/



    }



/*

    if( args.Length() >  0) row = args[0]->Int32Value();
    if( args.Length() >  1) col = args[1]->Int32Value();
    if( args.Length() >  2) type = args[2]->Int32Value();

    int id = Mat_ID;
    v8::Handle<v8::ObjectTemplate> mat_template = mat_t();

    Persistent<Object> obj = Persistent<Object>::New(mat_template->NewInstance());

    Mat *o;
    if( !row || !col) o = new Mat();
    else o = new Mat(row,col,type);

//    obj.MakeWeak(o, objectWeakCallback);
*/

//    obj->SetInternalField(0, External::New(m_vlcInstance));
//    obj->SetInternalField(1, External::New(m_vlcMedia));
//    obj->SetInternalField(2, External::New(m_vlcMediaplayer));

    obj->SetAlignedPointerInInternalField(0, m_vlcInstance);
    obj->SetAlignedPointerInInternalField(1, m_vlcMedia);
    obj->SetAlignedPointerInInternalField(2, m_vlcMediaplayer);


//    obj->SetInternalField(1, Int32::New(id));

    player_template.Clear();

    return handle_scope.Close(obj);
}

/////////////////////////////

static v8::Handle<v8::Value>frames_play(const Arguments &args)
{
    HandleScope handle_scope;

//    libvlc_media_player_t * player = (libvlc_media_player_t *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
    libvlc_media_player_t * player = (libvlc_media_player_t *)args.This()->GetAlignedPointerFromInternalField(2);
    if( player) libvlc_media_player_play(player);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>frames_pause(const Arguments &args)
{
    HandleScope handle_scope;

//    libvlc_media_player_t * player = (libvlc_media_player_t *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
    libvlc_media_player_t * player = (libvlc_media_player_t *)args.This()->GetAlignedPointerFromInternalField(2);
    if( player) libvlc_media_player_pause(player);

    return handle_scope.Close(Boolean::New(true));
}
static v8::Handle<v8::Value>frames_resume(const Arguments &args)
{
    HandleScope handle_scope;

//    libvlc_media_player_t * player = (libvlc_media_player_t *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
    libvlc_media_player_t * player = (libvlc_media_player_t *)args.This()->GetAlignedPointerFromInternalField(2);
    if( player) libvlc_media_player_set_pause(player,0);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>frames_stop(const Arguments &args)
{
    HandleScope handle_scope;

//    libvlc_media_player_t * player = (libvlc_media_player_t *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
    libvlc_media_player_t * player = (libvlc_media_player_t *)args.This()->GetAlignedPointerFromInternalField(2);

    if( player) libvlc_media_player_stop(player);

    return handle_scope.Close(Boolean::New(true));
}
static v8::Handle<v8::Value>frames_volume(const Arguments &args)
{
    HandleScope handle_scope;
    int vol = args[0]->Int32Value();

//    libvlc_media_player_t * player = (libvlc_media_player_t *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
    libvlc_media_player_t * player = (libvlc_media_player_t *)args.This()->GetAlignedPointerFromInternalField(2);

    if( player) libvlc_audio_set_volume(player,vol>10?vol:0);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>frames_pos(const Arguments &args)
{
    HandleScope handle_scope;
    float cpos = 0;

//    libvlc_media_player_t * player = (libvlc_media_player_t *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();
    libvlc_media_player_t * player = (libvlc_media_player_t *)args.This()->GetAlignedPointerFromInternalField(2);

    if( player) {
        if( args.Length() > 0) {
            double pos = args[0]->NumberValue();
            if( pos > 1) pos /= 100.;
            libvlc_media_player_set_position( player, (float) pos );
        }
        cpos = libvlc_media_player_get_position( player );
    }

    return handle_scope.Close(Number::New(cpos));
}

static v8::Handle<v8::Value>frames_del(const Arguments &args)
{
    HandleScope handle_scope;

//    libvlc_instance_t * instance = (libvlc_instance_t *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
//    libvlc_media_t * media = (libvlc_media_t *)(Handle<External>::Cast(args.This()->GetInternalField(1)))->Value();
//    libvlc_media_player_t * player = (libvlc_media_player_t *)(Handle<External>::Cast(args.This()->GetInternalField(2)))->Value();

    libvlc_instance_t * instance = (libvlc_instance_t *)args.This()->GetAlignedPointerFromInternalField(0);
    libvlc_media_t * media = (libvlc_media_t *)args.This()->GetAlignedPointerFromInternalField(1);
    libvlc_media_player_t * player = (libvlc_media_player_t *)args.This()->GetAlignedPointerFromInternalField(2);

    libvlc_media_player_stop(player);
    libvlc_media_player_release(player);
    libvlc_media_release(media);
    libvlc_release(instance);


    return handle_scope.Close(Boolean::New(true));
}


static v8::Handle<v8::ObjectTemplate>frames_t()
{
    HandleScope handle_scope;

    Handle<v8::ObjectTemplate> player_template = (Handle<v8::ObjectTemplate>)v8::ObjectTemplate::New();
    player_template->SetInternalFieldCount(3);

    player_template->Set(v8::String::NewSymbol("play"),v8::FunctionTemplate::New(frames_play)->GetFunction());
    player_template->Set(v8::String::NewSymbol("pause"),v8::FunctionTemplate::New(frames_pause)->GetFunction());
    player_template->Set(v8::String::NewSymbol("resume"),v8::FunctionTemplate::New(frames_resume)->GetFunction());
    player_template->Set(v8::String::NewSymbol("stop"),v8::FunctionTemplate::New(frames_stop)->GetFunction());
    player_template->Set(v8::String::NewSymbol("volume"),v8::FunctionTemplate::New(frames_volume)->GetFunction());
    player_template->Set(v8::String::NewSymbol("pos"),v8::FunctionTemplate::New(frames_pos)->GetFunction());

    player_template->Set(v8::String::NewSymbol("del"),v8::FunctionTemplate::New(frames_del)->GetFunction());

    return handle_scope.Close(player_template);
}

void* frm_lock(void *data, void **p_pixels)
{


    char *p = (char *)data;

//    printf("frm_lock %d\n",p[1]);fflush(stdout);

    if( !p[1])
        *p_pixels = p + 1024;
    else
        *p_pixels = p_pixels_null;


  return NULL;
}

//---------------- unlock -----------------//
// Функция вызывается библиотекой VLC из своего потока, когда принятый кадр записан в буфер
void frm_unlock(void *data, void *id, void *const *p_pixels)
{
    char *p = (char*)data;
    *p = 1;

    return;
}

//------------ display -----------------//
void  frm_display(void *data, void *id)
{
    /* VLC wants to display the video */
    (void) data;
}

static v8::Handle<v8::Value>frames(const Arguments &args)
{
    HandleScope handle_scope;
    int id = Player_ID;
    int i;
    libvlc_instance_t *m_vlcInstance = 0;
    libvlc_media_t *m_vlcMedia = 0;
    libvlc_media_player_t *m_vlcMediaplayer = 0;
    int nWidth = 640;
    int nHeight = 480;


    if( args.Length() < 2) return Boolean::New(false);
    String::Utf8Value ar(args[0]);
    String::Utf8Value fn(args[1]);

    int vlc_argc=0;
    char *vlc_argv[100];
    memset(vlc_argv,0,sizeof(vlc_argv));
    char *p = *ar;
    if( *p) {
        vlc_argv[vlc_argc++] = p;
        for(p++; *p; ++p) {
            if( *(p-1) == '\n') {
                vlc_argv[vlc_argc++] = p;
                *(p-1) = 0;
            }
        }
    }

    v8::Handle<v8::ObjectTemplate> player_template = frames_t();
    Persistent<Object> obj = Persistent<Object>::New(player_template->NewInstance());

    if( args.Length() > 2) {
        if( args.Length() > 3) nWidth = args[3]->Int32Value();
        if( args.Length() > 4) nHeight = args[4]->Int32Value();

        Local<Object> obj = args[2]->ToObject();

//        Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
//        void* o = field->Value();

        hBuf *  buf = (hBuf *  )obj->GetAlignedPointerFromInternalField(0);

        m_vlcInstance = libvlc_new(vlc_argc, vlc_argv);
//        m_vlcMedia = libvlc_media_new_path(m_vlcInstance, *fn);
        m_vlcMedia = libvlc_media_new_location(m_vlcInstance, *fn);
        m_vlcMediaplayer = libvlc_media_player_new_from_media(m_vlcMedia);

        libvlc_video_set_format(m_vlcMediaplayer, "RV32", nWidth, nHeight, nWidth*4);
        libvlc_video_set_callbacks(m_vlcMediaplayer, frm_lock, frm_unlock, frm_display, buf->p);/////!!!!

        libvlc_media_player_play(m_vlcMediaplayer);
    }

    obj->SetAlignedPointerInInternalField(0, m_vlcInstance);
    obj->SetAlignedPointerInInternalField(1, m_vlcMedia);
    obj->SetAlignedPointerInInternalField(2, m_vlcMediaplayer);

    player_template.Clear();

    return handle_scope.Close(obj);
}


///////////////////////////////////////////////
static v8::Handle<v8::Value>saver_del(const Arguments &args)
{
    HandleScope handle_scope;

//    libvlc_instance_t * instance = (libvlc_instance_t *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
    libvlc_instance_t * instance = (libvlc_instance_t *)args.This()->GetAlignedPointerFromInternalField(0);

    if( !instance) return handle_scope.Close(Boolean::New(false));

    libvlc_vlm_stop_media(instance,media_name);
    libvlc_vlm_del_media(instance,media_name);
    libvlc_vlm_release(instance);

    args.This()->SetAlignedPointerInInternalField(0, 0);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>saver_file(const Arguments &args)
{
    HandleScope handle_scope;
    if( args.Length() < 1) return Boolean::New(false);
    String::Utf8Value fn(args[0]);

//    libvlc_instance_t * instance = (libvlc_instance_t *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
    libvlc_instance_t * instance = (libvlc_instance_t *)args.This()->GetAlignedPointerFromInternalField(0);
    if( !instance) return handle_scope.Close(Boolean::New(false));

    libvlc_vlm_stop_media( instance, media_name);
    libvlc_vlm_set_output( instance, media_name,*fn);
    libvlc_vlm_play_media( instance, media_name);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>saver_stop(const Arguments &args)
{
    HandleScope handle_scope;

//    libvlc_instance_t * instance = (libvlc_instance_t *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
    libvlc_instance_t * instance = (libvlc_instance_t *)args.This()->GetAlignedPointerFromInternalField(0);
    if( !instance) return handle_scope.Close(Boolean::New(false));

    libvlc_vlm_stop_media( instance, media_name);
//    libvlc_vlm_play_media( instance, media_name);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>saver_play(const Arguments &args)
{
    HandleScope handle_scope;

//    libvlc_instance_t * instance = (libvlc_instance_t *)(Handle<External>::Cast(args.This()->GetInternalField(0)))->Value();
    libvlc_instance_t * instance = (libvlc_instance_t *)args.This()->GetAlignedPointerFromInternalField(0);
    if( !instance) return handle_scope.Close(Boolean::New(false));

//    libvlc_vlm_stop_media( instance, media_name);
    libvlc_vlm_play_media( instance, media_name);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::ObjectTemplate>saver_t()
{
    HandleScope handle_scope;

    Handle<v8::ObjectTemplate> player_template = (Handle<v8::ObjectTemplate>)v8::ObjectTemplate::New();
    player_template->SetInternalFieldCount(1);

    player_template->Set(v8::String::NewSymbol("file"),v8::FunctionTemplate::New(saver_file)->GetFunction());
    player_template->Set(v8::String::NewSymbol("stop"),v8::FunctionTemplate::New(saver_stop)->GetFunction());
    player_template->Set(v8::String::NewSymbol("play"),v8::FunctionTemplate::New(saver_play)->GetFunction());
    player_template->Set(v8::String::NewSymbol("del"),v8::FunctionTemplate::New(saver_del)->GetFunction());

    return handle_scope.Close(player_template);
}

int vlc_save_stopped_f = 0;

static v8::Handle<v8::Value>is_stopped(const Arguments &args)
{
    HandleScope handle_scope;

    return handle_scope.Close(Boolean::New(vlc_save_stopped_f));
}

void vlc_callbacks( const libvlc_event_t* event, void* ptr )
{

        switch ( event->type )
        {
//        libvlc_VlmMediaInstanceStatusOpening
        case libvlc_VlmMediaInstanceStatusOpening:
            fprintf(stderr,"------------------------------------ libvlc_VlmMediaInstanceStatusOpening \n");fflush(stderr);
            break;
        case libvlc_VlmMediaRemoved:
            fprintf(stderr,"------------------------------------ libvlc_VlmMediaRemoved \n");fflush(stderr);
            break;
        case libvlc_VlmMediaInstanceStatusError:
            fprintf(stderr,"------------------------------------ libvlc_VlmMediaInstanceStatusError \n");fflush(stderr);
            break;
        case libvlc_VlmMediaInstanceStopped://!!!!!
            fprintf(stderr,"------------------------------------ libvlc_VlmMediaInstanceStopped \n");fflush(stderr);
            vlc_save_stopped_f = -1;
            break;
        case libvlc_VlmMediaInstanceStatusEnd:
            fprintf(stderr,"------------------------------------ libvlc_VlmMediaInstanceStatusEnd \n");fflush(stderr);
            break;
        case libvlc_VlmMediaInstanceStatusPlaying:
            fprintf(stderr,"------------------------------------ libvlc_VlmMediaInstanceStatusPlaying \n");fflush(stderr);
            break;
        default:
            fprintf(stderr,"------------------!!!!!!!!!!!!!!!!!!!!!!!!! %d %X\n",event->type,event->type);fflush(stderr);

        }
}

static v8::Handle<v8::Value>saver(const Arguments &args)
{
    HandleScope handle_scope;
    int id = Player_ID;
    int i;
    libvlc_instance_t *m_vlcInstance = 0;

    if( args.Length() < 3) return Boolean::New(false);
    String::Utf8Value ar(args[0]);
    String::Utf8Value fn(args[1]);
    String::Utf8Value sfn(args[2]);

    int add_argc=0;
    char *add_argv[100];
    memset(add_argv,0,sizeof(add_argv));

    vlc_save_stopped_f = 0;

    char *padd = strchr(*ar,'|');
/*
    if( !padd) {
        padd = "-b\n8300000\n-B\n8300000\n-v";
    }
*/
    if( padd){
        *padd = 0;
        ++padd;
        if( *padd) {
            add_argv[add_argc++] = padd;
            for(padd++; *padd; ++padd) {
                if( *(padd-1) == '\n') {
                    add_argv[add_argc++] = padd;
                    *(padd-1) = 0;
                }
            }
        }
    }

    int vlc_argc=0;
    char *vlc_argv[100];
    memset(vlc_argv,0,sizeof(vlc_argv));
    char *p = *ar;
    if( *p) {
        vlc_argv[vlc_argc++] = p;
        for(p++; *p; ++p) {
            if( *(p-1) == '\n') {
                vlc_argv[vlc_argc++] = p;
                *(p-1) = 0;
            }
        }
    }

    v8::Handle<v8::ObjectTemplate> player_template = saver_t();
    Persistent<Object> obj = Persistent<Object>::New(player_template->NewInstance());

    m_vlcInstance = libvlc_new(vlc_argc, vlc_argv);
    int ret = libvlc_vlm_add_broadcast(m_vlcInstance,media_name,*fn,*sfn,add_argc,add_argv,1,0);
    printf("libvlc_vlm_add_broadcast %d\n",ret);fflush(stdout);
    ret = libvlc_vlm_play_media(m_vlcInstance,media_name);
    printf("libvlc_vlm_play_media %d\n",ret);fflush(stdout);

    libvlc_event_manager_t* em = libvlc_vlm_get_event_manager( m_vlcInstance ) ;

    libvlc_event_attach( em, libvlc_VlmMediaAdded,   vlc_callbacks, m_vlcInstance );
    libvlc_event_attach( em, libvlc_VlmMediaRemoved,   vlc_callbacks, m_vlcInstance );
    libvlc_event_attach( em, libvlc_VlmMediaChanged,   vlc_callbacks, m_vlcInstance );
    libvlc_event_attach( em, libvlc_VlmMediaInstanceStarted,   vlc_callbacks, m_vlcInstance );
    libvlc_event_attach( em, libvlc_VlmMediaInstanceStopped,   vlc_callbacks, m_vlcInstance );
    libvlc_event_attach( em, libvlc_VlmMediaInstanceStatusInit,   vlc_callbacks, m_vlcInstance );
    libvlc_event_attach( em, libvlc_VlmMediaInstanceStatusOpening,   vlc_callbacks, m_vlcInstance );
    libvlc_event_attach( em, libvlc_VlmMediaInstanceStatusPlaying,   vlc_callbacks, m_vlcInstance );
    libvlc_event_attach( em, libvlc_VlmMediaInstanceStatusPause,   vlc_callbacks, m_vlcInstance );
    libvlc_event_attach( em, libvlc_VlmMediaInstanceStatusEnd,   vlc_callbacks, m_vlcInstance );
    libvlc_event_attach( em, libvlc_VlmMediaInstanceStatusError,   vlc_callbacks, m_vlcInstance );

    obj->SetAlignedPointerInInternalField(0, m_vlcInstance);

    player_template.Clear();

    return handle_scope.Close(obj);
}


//v8::Handle<v8::Value> init(v8::Persistent<v8::Context> ctx)
void vlc_init()
//extern "C" MY_EXPORT v8::Handle<v8::Value>  init(v8::Persistent<v8::Context> ctx)
{
    HandleScope handle_scope;
/*
    cvw_template = (Persistent<v8::ObjectTemplate>)v8::ObjectTemplate::New();
    cvw_template->SetInternalFieldCount(2);

    mat_template = (Persistent<v8::ObjectTemplate>)v8::ObjectTemplate::New();
    mat_template->SetInternalFieldCount(2);
*/
    Handle<Object> global = context->Global();

    Handle<Object> obj = Object::New();

    obj->Set(v8::String::NewSymbol("x"),Int32::New(40));
    obj->Set(v8::String::NewSymbol("y"),Int32::New(9999));

    obj->Set(v8::String::NewSymbol("player"),v8::FunctionTemplate::New(player)->GetFunction());//fn
    obj->Set(v8::String::NewSymbol("saver"),v8::FunctionTemplate::New(saver)->GetFunction());//fn
    obj->Set(v8::String::NewSymbol("frames"),v8::FunctionTemplate::New(frames)->GetFunction());//fn
    obj->Set(v8::String::NewSymbol("stopped"),v8::FunctionTemplate::New(is_stopped)->GetFunction());//fn

    global->Set(v8::String::New("vlc"), obj);

//    return handle_scope.Close(obj);
}
