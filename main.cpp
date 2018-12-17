#include <QtGui/QApplication>
#include "mainwindow.h"
#include "hh_curl_util.h"
#include "hio.h"
#include "gui.h"
#include "bb.h"

//v8::HandleScope handle_scope;
v8::Persistent<v8::Context> context;
//Handle<Object> global;

using namespace v8;

char   *glb_arr[100] = {"qwert","","",0};

//#include "opencv2/objdetect/objdetect.hpp"
//#include "opencv2/imgproc/imgproc.hpp"
//#include "opencv2/highgui/highgui.hpp"
//using namespace cv;

MainWindow *w;
//CURL *curl_handle;

int quit_flg = false;
int close_flg = false;

char * get_file_text_utf8(char * fn,int *fsz);
char * get_resource(char * fn,int *fsz);

#ifdef _WIN32
#else
#include <signal.h>
#include <sys/wait.h>

void qhdl(int sig)
{
    fprintf(stderr,"SIGNAL %d !\n",sig);fflush(stderr);
//    FCGX_ShutdownPending();
    quit_flg = true;
}

int run_process = 0;
/*
void qhdl_SIGCHLD(int sig)
{
    fprintf(stderr,"SIGNAL %d !\n",sig);fflush(stderr);
    int stat_loc;

    pid_t p = wait( &stat_loc);

    run_process = 0;

    fprintf(stderr,"EXIT PROCESS %d Status %d\n",p,stat_loc);fflush(stderr);
}
*/
void qhdl_PIPE(int sig)
{
    fprintf(stderr,"SIGNAL qhdl_PIPE %d !\n",sig);fflush(stderr);
//    FCGX_ShutdownPending();
//    quit_flg = true;
}

void qhdl_SIGCHLD(int sig)
{
    fprintf(stderr,"SIGNAL %d !\n",sig);fflush(stderr);
    int stat_loc;

//    pid_t p = wait( &stat_loc);
    while( 1) {
        pid_t p = waitpid( -1, &stat_loc,WNOHANG);
        if( p <= 0) {
            break;
        }
        fprintf(stderr,"EXIT PROCESS %d Status %d\n",p,stat_loc);fflush(stderr);
    }

    run_process = 0;
}

#endif

QString rt(const char * s)
{
    static QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    return(codec->toUnicode(s));
}

char * rd(QString s)
{
    static QByteArray ba;
    static QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    ba = codec->fromUnicode(s);

    return(ba.data());
}
void Usage(char *programName)
{
    /* Modify here to add your usage message when the program is
     * called without arguments */

    fprintf(stderr,"%s usage:\n",programName);
    fprintf(stderr,"%s [-<options>] \n",programName);
    fprintf(stderr,"options:\n");

//    fprintf(stderr,"\t-S<SPHINX server_name(localhost)>\n");

    fprintf(stderr,"\t-?\tHELP\n");
}

char f_in[1000] = "in.js";
char silent_flg = 0;
char url_str[100] = "";

int HandleOptions(int argc,char *argv[])
{
    int i,firstnonoption=0;

    for (i=1; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
                /* An argument -? means help is requested */
            case '?':
                Usage(argv[0]);
                break;

            case 's':
                if( !strcmpi("silent",&argv[i][1])) silent_flg = 1;
                break;

            case 'U':
                if( !strcmpi("URL",&argv[i][1]) && (i < (argc-1))){
                    strcpy(url_str,argv[++i]);
                }

                break;

            default:
//                fprintf(stderr,"unknown option %s\n",argv[i]);


                break;
            }
        }
        else {
            firstnonoption = i;
            strcpy(f_in,argv[i]);
//			break;
        }
    }
    return firstnonoption;
}

const int KB = 1024;
const int MB = KB * KB;
const int GB = KB * KB * KB;
const int kPointerSize   = sizeof(void*);     // NOLINT

int argc_;
char **argv_;

int main(int argc, char *argv[])
{
//    v8::ResourceConstraints::set_max_old_space_size();

    argc_ = argc;
    argv_ = argv;

#ifdef _WIN32
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    wVersionRequested = MAKEWORD( 2, 2 );

    err = WSAStartup( wVersionRequested, &wsaData );
    if ( err != 0 ) {
    }

    /* Confirm that the WinSock DLL supports 2.2.*/
    /* Note that if the DLL supports versions later    */
    /* than 2.2 in addition to 2.2, it will still return */
    /* 2.2 in wVersion since that is the version we      */
    /* request ed.                                        */

    if ( LOBYTE( wsaData.wVersion ) != 2 ||
            HIBYTE( wsaData.wVersion ) != 2 ) {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */
        fprintf(stderr,"Tell the user that we could not find a usable WinSock DLL. \n");fflush(stderr);
//		WSACleanup( );
//		return;
    }
#endif

    HandleOptions(argc,argv);

#ifdef _WIN32
#else
    signal(SIGTERM, qhdl);
    signal(SIGINT, qhdl);
    signal(SIGTSTP, qhdl);
    signal(SIGCHLD, qhdl_SIGCHLD);
    signal(SIGPIPE, qhdl_PIPE);

#endif


//    curl_handle = curl_easy_init();
//    curl_global_init(CURL_GLOBAL_ALL);
    curl_global_init(CURL_GLOBAL_ALL);

    Q_INIT_RESOURCE(hjs);
    int ret = 0;
    QTextCodec* codec =  QTextCodec::codecForName("UTF-8");
    QTextCodec::setCodecForTr(codec);
    QTextCodec::setCodecForCStrings(codec);

    QApplication a(argc, argv);
    a.addLibraryPath(a.applicationDirPath() + "/plugins");
    a.addLibraryPath("/opt/plugins");

//    fprintf(stderr,"dir $ %s\n",rd(a.applicationDirPath()));fflush(stderr);
//    fprintf(stderr,"dir_lib $ %s\n",rd(a.libraryPaths()));fflush(stderr);
    QStringList pl = a.libraryPaths();
    for(int i = 0; i < pl.length(); ++i){
//        fprintf(stderr,"dir_lib $ %s\n",rd(pl[i]));fflush(stderr);
    }

//    MainWindow mw;
    w = new MainWindow();
    Qt::WindowFlags flags = 0;
    flags = Qt::Window;
    flags |= Qt::CustomizeWindowHint;
    flags |= Qt::WindowTitleHint;
    flags |= Qt::WindowSystemMenuHint;
    flags |= Qt::WindowMinimizeButtonHint;
    w->setWindowFlags(flags);

    int lump_of_memory = (kPointerSize / 4) * MB;
    v8::ResourceConstraints constraints;


    constraints.set_max_young_space_size(16 * lump_of_memory);
    constraints.set_max_old_space_size(1000 * lump_of_memory);
//    constraints.set_max_executable_size(256 * lump_of_memory);

//    v8::SetResourceConstraints(&constraints);

//    fprintf(stderr,"max_old_space_size: %d\n",constraints.max_old_space_size());fflush(stderr);

    v8::HandleScope handle_scope;

//    v8::Persistent<v8::Context> context = v8::Context::New();
    context = v8::Context::New();


//    fprintf(stderr,"context:: %ld\n",glb_arr);fflush(stderr);

    v8::Context::Scope context_scope(context);

//    global = context->Global();



    v8::TryCatch trycatch;
/*
    v8::Handle<v8::String> source = v8::String::New("'Hello' + ', World! Всем Здравствуйте'");
    v8::Handle<v8::Script> script = v8::Script::Compile(source);
    v8::Handle<v8::Value> result = script->Run();

    v8::String::Utf8Value ascii(result);
    fprintf(stderr,"%s\n", *ascii);fflush(stderr);
*/
    Handle<Object> global = context->Global();
    Handle<Array> ar = Array::New();
    for( int i = 0; i < argc; ++i) {
        ar->Set(v8::Number::New(i),String::New(argv[i]));
    }
    global->Set(String::New("ARGV"), ar);

    new bb();
    new io();
    new gui();

    char * inf = get_resource(f_in,0);
    if( inf) {
//        v8::Handle<v8::Value> result = v8::Script::Compile(v8::String::New(inf))->Run();
        Local<Value> result;
        Local<Script> script = Script::Compile(v8::String::New(inf)); // компилируем
        if (trycatch.HasCaught()) {
            String::Utf8Value err(trycatch.Message()->Get());
            fprintf(stderr,"COMPILE ERROR: %s\n",*err);fflush(stderr);
            String::Utf8Value exc(trycatch.Exception());
            fprintf(stderr,"COMPILE ERROR - EXEPTION: %s\n",*exc);fflush(stderr);
            String::Utf8Value st(trycatch.StackTrace());
            fprintf(stderr,"COMPILE ERROR - STACK: %s\n",*st);fflush(stderr);
            goto oop;
        }
        result = script->Run(); // выполняем
        if (trycatch.HasCaught()) {
            String::Utf8Value err(trycatch.Message()->Get());
            fprintf(stderr,"RUN ERROR: %s\n",*err);fflush(stderr);
            String::Utf8Value exc(trycatch.Exception());
            fprintf(stderr,"RUN ERROR - EXEPTION: %s\n",*exc);fflush(stderr);
            String::Utf8Value st(trycatch.StackTrace());
            fprintf(stderr,"RUN ERROR - STACK: %s\n",*st);fflush(stderr);
        }
oop:
        free(inf);
    }

//    w->show();

/*
    cv::VideoCapture vc;
    int rr = vc.open("car.avi");

    fprintf(stderr,"VideoCaptureFnc!!! %d\n",rr);fflush(stderr);
*/

    if( !w->isHidden() || silent_flg) {
        ret = a.exec();
    }

    context.Dispose();

//    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();

//    fprintf(stderr,"main 99\n");fflush(stderr);

#ifdef _WIN32
    WSACleanup();
#endif

    return ret;
}
