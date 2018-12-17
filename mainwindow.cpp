#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "gui.h"
using namespace v8;

extern v8::Persistent<v8::Context> context;
extern int close_flg;


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    vl = ui->vl;
    vl->setSpacing(4);
    cur_Menu = 0;
    cur_ToolBar = ui->mainToolBar;
    addToolBar(Qt::LeftToolBarArea,cur_ToolBar);

    in_t.start();
    infined_t.start();

    setWindowIcon(QIcon(":hjs.gif"));
    setWindowTitle("Hyper JS");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::action_do()
{
    HandleScope handle_scope;

    if( close_flg) return;

    Handle<v8::Object> global = context->Global();

    hAction * o = (hAction * )sender();

    Handle<Value> args[2];
    args[0] = v8::Int32::New(o->foo);
    args[1] = v8::Int32::New(o->id);
    Handle<Value> js_result = o->fnc->Call(global, 2, args);
}

void MainWindow::button_do()
{
    HandleScope handle_scope;

    if( close_flg) return;

    Handle<v8::Object> global = context->Global();

    hButton * o = (hButton * )sender();

    Handle<Value> args[2];
    args[0] = v8::Int32::New(o->foo);
    args[1] = v8::Int32::New(o->id);

    Handle<Value> js_result = o->fnc->Call(global, 2, args);
/*
Handle<v8::Value> value = global->Get(String::New("ff"));

if (value->IsFunction()) {
    Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(value);

    Handle<Value> js_result = func->Call(global, 2, args);
}
*/
}

void MainWindow::tree_do(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    HandleScope handle_scope;

    if( close_flg) return;

    Handle<v8::Object> global = context->Global();

    hTree * o = (hTree * )sender();
    Handle<Value> args[2];
    args[0] = v8::Int32::New(o->foo);
    args[1] = v8::Int32::New(o->id);
    Handle<Value> js_result = o->fnc->Call(global, 2, args);
}

void MainWindow::list_do(QListWidgetItem * current, QListWidgetItem * previous )
{
    HandleScope handle_scope;

    if( close_flg) return;

    Handle<v8::Object> global = context->Global();

    hList * o = (hList * )sender();
    Handle<Value> args[2];
    args[0] = v8::Int32::New(o->foo);
    args[1] = v8::Int32::New(o->id);
    Handle<Value> js_result = o->fnc->Call(global, 2, args);
}

void MainWindow::table_do(QTableWidgetItem * current, QTableWidgetItem * previous )
{
    HandleScope handle_scope;

    if( close_flg) return;

    Handle<v8::Object> global = context->Global();

    hTable * o = (hTable * )sender();
    Handle<Value> args[2];
    args[0] = v8::Int32::New(o->foo);
    args[1] = v8::Int32::New(o->id);
    Handle<Value> js_result = o->fnc->Call(global, 2, args);
}

void MainWindow::timer_do()
{
    HandleScope handle_scope;

//    if( quit_flg) return;

    Handle<v8::Object> global = context->Global();

    hTimer * o = (hTimer * )sender();
    Handle<Value> args[2];
    args[0] = v8::Int32::New(o->foo);
    args[1] = v8::Int32::New(o->id);
    Handle<Value> js_result = o->fnc->Call(global, 2, args);
}

extern int argc_;
extern char **argv_;

void pcloning()
{
    char *a[200];
    int ac = argc_;
    memset(a,0,sizeof(a));
    int f_ret = 0;

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
/*
        if( args.Length() > 0 ){
            ::close(0);
            ::close(1);
            ::close(2);
        }

        if( args.Length() > 1 ) {
            String::Utf8Value d(args[1]);
            chdir(*d);
        }
*/
        setsid();
        execvp(a[0],a);
//        setsid();
    }
/*
    if( args.Length() > 0 ) {
        for(int i=0; a[i] && i < 200; ++i){
            free(a[i]);
        }
    }
*/
}

void InfinedThread_T::run()
{
    while( true) {
        time_t tml = tm;
        time_t tc = time(0);

        if( tm_clone && tm_clone < tc){
            pcloning();

            tm_clone = 0;
        }

        if( tml == 0) {
            msleep(150);
            continue;
        }
        if( tml < tc){
            if( par[0] != 0) {
//                char prs[3000] = "Нечто";
//                fprintf(stderr,"\nexecv PROCESS! %s %s\n",prs,par);fflush(stderr);

                char *a[200];
                int ac = argc_;
                memset(a,0,sizeof(a));
                int f_ret = 0;

                int i = 0;
                for(i=0; i < argc_; ++i) {
//                    fprintf(stderr,"%s\n",argv_[i]);fflush(stderr);
                    if( !strncmp(argv_[i],"-R",2)){
                        a[i] = par;
                        f_ret = 1;
                        continue;
                    }
                    a[i] = argv_[i];
                }

                if( !f_ret) {
                    a[i] = par;
                    ac++;
                }
/*
                for(int j=0; j < ac; ++j) {
                    fprintf(stderr,"%s\n",a[j]);fflush(stderr);
                }
*/

                if( !fork()) {
//                    fprintf(stderr,"\nIn new execv PROCESS! %s %s\n",prs,par);fflush(stderr);
                    execvp(a[0],a);
//                    fprintf(stderr,"\nIn new execv PROCESS! %s %s\n",prs,par);fflush(stderr);
                    return;
                }


            }

            fprintf(stderr,"\nKILLED BY TIMEOUT! %d %d \n",tml , tc);fflush(stderr);
            _exit(-1);
            return;
        }
        msleep(150);
    }
};
