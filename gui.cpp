#include "gui.h"

using namespace v8;
Persistent<v8::ObjectTemplate> gui_template;
QString rt(const char * s);
char * rd(QString s);

extern MainWindow *w;
extern v8::Persistent<v8::Context> context;
//extern Handle<Object> global;
void save_resource(char * fn);

int foo_cnt = 0;
QIcon *prg_ic=0;
//extern int quit_flg;
extern int close_flg;

enum{Button_ID=10000,Action_ID,Vbox_ID,Hbox_ID,Glay_ID
,Space_ID,Check_ID,Radio_ID,ButtonGroup_ID
,Label_ID,Lcd_ID,Progress_ID,Combo_ID,Line_ID,Text_ID,PText_ID,Spin_ID,Dial_ID,Dttm_ID,Hslider_ID,Vslider_ID,Web_ID
,Tab_ID
,Table_ID,Tree_ID,TreeItem_ID,List_ID
,Timer_ID
,Dialog_ID
,EXTERNAL_ID

};

#define GUI_SPACE 4

hTimer::hTimer( ): QTimer()
{
    id = Timer_ID;
    foo = foo_cnt++;
}

hButton::hButton( QString text): QPushButton(text)
{
//    setIcon(QIcon("Run3.PNG"));
    id = Button_ID;
    foo = foo_cnt++;
}

bool hButton::event( QEvent * e )
{
//    fprintf(stderr,">>%d\n",e->type());fflush(stderr);
    QPushButton::event(e);
/*
    if(e->type() == 2 || e->type() == 4) {
        QMouseEvent *me = (QMouseEvent *)e;
        fprintf(stderr,"%d\n",e->type());fflush(stderr);
        fprintf(stderr,"me %d\n",me->button());fflush(stderr);


    }
*/
    return(true);
}

hTable::hTable( ): QTableWidget()
{
    id = Table_ID;
    foo = foo_cnt++;
}

hTree::hTree( ): QTreeWidget()
{
    id = Tree_ID;
    foo = foo_cnt++;
}

hList::hList( ): QListWidget()
{
    id = List_ID;
    foo = foo_cnt++;
}

hAction::hAction( QString text,QObject * parent): QAction(text,parent)
{
    id = Action_ID;
    foo = foo_cnt++;
}

hAction::hAction( QObject * parent): QAction(parent)
{
    id = Action_ID;
    foo = foo_cnt++;

}

hAction::hAction( const QIcon  icon,QString text,QObject * parent): QAction(icon,text,parent)
{
    id = Action_ID;
    foo = foo_cnt++;
}
void hAction::trigger()
{
    printf("trigger >\n");fflush(stderr);
}

bool hAction::event( QEvent * e )
{
    QAction::event(e);

//    fprintf(stderr,"act > %d\n",e->type());fflush(stderr);
/*
    QPushButton::event(e);

    if(e->type() == 2 || e->type() == 4) {
        QMouseEvent *me = (QMouseEvent *)e;
        fprintf(stderr,"%d\n",e->type());fflush(stderr);
        fprintf(stderr,"me %d\n",me->button());fflush(stderr);


    }
*/
    return(true);
}


hGreed::hGreed( int c): QGridLayout()
{
    col = c;
    col_cnt = 0;
}

static void addWidget(void * wd,int w_id)
{
//    fprintf(stderr,"addWidget > %d %d\n",w->cur_lay,w->cur_lay_type);fflush(stderr);

    switch(w->cur_lay_type) {

    case Vbox_ID:
        switch( w_id) {
        case Vbox_ID:
        case Hbox_ID:
        case Glay_ID:
            ((QVBoxLayout *)w->cur_lay)->addLayout((QLayout *)wd);
            break;
        default:
            ((QVBoxLayout *)w->cur_lay)->addWidget((QWidget *)wd);
        }
        break;

    case Hbox_ID:
        switch( w_id) {
        case Vbox_ID:
        case Hbox_ID:
        case Glay_ID:
            ((QHBoxLayout *)w->cur_lay)->addLayout((QLayout *)wd);
            break;
        default:
            ((QHBoxLayout *)w->cur_lay)->addWidget((QWidget *)wd);
        }
        break;

    case Glay_ID:
        hGreed * g = (hGreed * )w->cur_lay;
        int row = g->col_cnt / g->col;
        int col = g->col_cnt % g->col;

//        fprintf(stderr,"row %d col %d\n",row,col);fflush(stderr);

        switch( w_id) {
        case Vbox_ID:
        case Hbox_ID:
        case Glay_ID:
            g->addLayout((QLayout *)wd,row,col);
            break;
        default:
            g->addWidget((QWidget *)wd,row,col);
        }

        ++g->col_cnt;
        break;
    }
}

static v8::Handle<v8::Value>button(const Arguments &args)
{
    HandleScope handle_scope;

    int id = Button_ID;
    Persistent<Object> obj = Persistent<Object>::New(gui_template->NewInstance());
    hButton * o = new hButton(0);
    if( args.Length() > 0) {
        String::Utf8Value ascii(args[0]);
        o->setText(rt(*ascii));
    }
    if( args.Length() > 1) {
        v8::Local<v8::Function> fnc =  v8::Local<v8::Function>::Cast(args[1]);
        o->fnc = v8::Persistent<v8::Function>::New(fnc);
/*
        Handle<Value> args[2];
        args[0] = v8::Int32::New(o->foo);
        args[1] = v8::Int32::New(o->id);

        o->fnc->Call(context->Global(), 2, args);
*/
    }

    if( args.Length() > 2) {
        String::Utf8Value ascii(args[2]);
        save_resource(*ascii);
        o->setIcon(QIcon(rt(*ascii)));
    }
    if( args.Length() > 3) {
        String::Utf8Value val(args[3]);
        o->setToolTip(*val);
        o->setStatusTip(*val);
    }
    if( args.Length() > 4) {
        o->foo = args[4]->Int32Value();
    }
    if( args.Length() > 5) {
        o->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    }

    addWidget(o,id);

    MainWindow::connect(o, SIGNAL(clicked()), w, SLOT(button_do()));

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>lab(const Arguments &args)
{
    HandleScope handle_scope;

    int xsz= false;
    int ysz = false;

    int id = Label_ID;
    Persistent<Object> obj = Persistent<Object>::New(gui_template->NewInstance());
    QLabel * o = new QLabel();
    if( args.Length() > 0) {
        String::Utf8Value ascii(args[0]);
        o->setText(*ascii);
    }
    if( args.Length() > 1) {
        String::Utf8Value ascii(args[1]);
//        QPicture * pic = new QPicture();
//        pic->load(*ascii);
        save_resource(*ascii);
        o->setPixmap(QPixmap(rt(*ascii)));
    }
    if( args.Length() > 2) {
        xsz = args[2]->Int32Value();
    }
    if( args.Length() > 3) {
        ysz = args[3]->Int32Value();
    }

    o->setSizePolicy(xsz ? QSizePolicy::Expanding:QSizePolicy::Fixed,ysz ? QSizePolicy::Expanding:QSizePolicy::Fixed);
    o->setTextInteractionFlags(Qt::TextSelectableByMouse);

    addWidget(o,id);

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>lcd(const Arguments &args)
{
    HandleScope handle_scope;
    int id = Lcd_ID;
    Persistent<Object> obj = Persistent<Object>::New(gui_template->NewInstance());
    QLCDNumber * o = new QLCDNumber();
    int32_t col = 4;
    int32_t val = 0;
    if( args.Length() > 0) {
        col = args[0]->Int32Value();
        o->setDigitCount(col);
    }
    if( args.Length() > 1) {
        val = args[1]->Int32Value();
        o->display(val);
    }

    o->setMinimumHeight(40);
    o->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);

    addWidget(o,id);

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>prog(const Arguments &args)
{
    HandleScope handle_scope;

    int id = Progress_ID;
    Persistent<Object> obj = Persistent<Object>::New(gui_template->NewInstance());
    QProgressBar * o = new QProgressBar();
    o->setMinimum(0);
    o->setMaximum(100);
    if( args.Length() > 0) {
        int32_t zn = args[0]->Int32Value();
        o->setValue(zn);
    }

//    o->setMinimumHeight(40);
//    o->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);

    addWidget(o,id);

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>combo(const Arguments &args)
{
    HandleScope handle_scope;

    int id = Combo_ID;
    Persistent<Object> obj = Persistent<Object>::New(gui_template->NewInstance());
    QComboBox * o = new QComboBox();

    if( args.Length() > 0) {
        String::Utf8Value val(args[0]);
        o->setToolTip(*val);
        o->setStatusTip(*val);
    }

    o->setEditable(true);

    addWidget(o,id);

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>line(const Arguments &args)
{
    HandleScope handle_scope;

    int id = Line_ID;
    Persistent<Object> obj = Persistent<Object>::New(gui_template->NewInstance());
    QLineEdit * o = new QLineEdit();

    if( args.Length() > 0) {
        String::Utf8Value val(args[0]);
        o->setToolTip(*val);
        o->setStatusTip(*val);
    }

    addWidget(o,id);

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>text(const Arguments &args)
{
    HandleScope handle_scope;

    int id = Text_ID;
    Persistent<Object> obj = Persistent<Object>::New(gui_template->NewInstance());
    QTextEdit * o = new QTextEdit();

    if( args.Length() > 0) {
        String::Utf8Value val(args[0]);
        o->setToolTip(*val);
        o->setStatusTip(*val);
    }

    if( args.Length() > 1) {
        int h = args[1]->Int32Value();
        o->setMaximumHeight(h);
    }

    addWidget(o,id);

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>ptext(const Arguments &args)
{
    HandleScope handle_scope;

    int id = PText_ID;
    Persistent<Object> obj = Persistent<Object>::New(gui_template->NewInstance());
    QPlainTextEdit * o = new QPlainTextEdit();

    if( args.Length() > 0) {
        String::Utf8Value val(args[0]);
        o->setToolTip(*val);
        o->setStatusTip(*val);
    }

    addWidget(o,id);

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>spin(const Arguments &args)
{
    HandleScope handle_scope;

    int id = Spin_ID;
    Persistent<Object> obj = Persistent<Object>::New(gui_template->NewInstance());
    QSpinBox * o = new QSpinBox();

    if( args.Length() > 0) {
        o->setMaximum(args[0]->Int32Value());
    }
    if( args.Length() > 1) {
        o->setMinimum(args[1]->Int32Value());
    }
    if( args.Length() > 2) {
        o->setSingleStep(args[2]->Int32Value());
    }

    addWidget(o,id);

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>dial(const Arguments &args)
{
    HandleScope handle_scope;

    int id = Dial_ID;
    Persistent<Object> obj = Persistent<Object>::New(gui_template->NewInstance());
    QDial * o = new QDial();

    if( args.Length() > 0) {
        o->setMaximum(args[0]->Int32Value());
    }
    if( args.Length() > 1) {
        o->setMinimum(args[1]->Int32Value());
    }
    if( args.Length() > 2) {
        o->setSingleStep(args[2]->Int32Value());
    }

    o->setMinimumHeight(80);
    o->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);


    addWidget(o,id);

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>dttm(const Arguments &args)
{
    HandleScope handle_scope;

    int id = Dttm_ID;
    Persistent<Object> obj = Persistent<Object>::New(gui_template->NewInstance());
    QDateTimeEdit * o = new QDateTimeEdit();
    o->setCalendarPopup(true);

/*
    if( args.Length() > 0) {
        o->setMaximum(args[0]->Int32Value());
    }
    if( args.Length() > 1) {
        o->setMinimum(args[1]->Int32Value());
    }
    if( args.Length() > 2) {
        o->setSingleStep(args[2]->Int32Value());
    }

    o->setMaximumHeight(80);
*/
    o->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);


    addWidget(o,id);

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>hslider(const Arguments &args)
{
    HandleScope handle_scope;

    int id = Hslider_ID;
    Persistent<Object> obj = Persistent<Object>::New(gui_template->NewInstance());
    QSlider * o = new QSlider(Qt::Horizontal);

    if( args.Length() > 0) {
        o->setMaximum(args[0]->Int32Value());
    }
    if( args.Length() > 1) {
        o->setMinimum(args[1]->Int32Value());
    }
    if( args.Length() > 2) {
        o->setSingleStep(args[2]->Int32Value());
    }

//    o->setMinimumWidth(160);
//    o->setMaximumWidth(160);

//    o->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);

    addWidget(o,id);

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>vslider(const Arguments &args)
{
    HandleScope handle_scope;

    int id = Vslider_ID;
    Persistent<Object> obj = Persistent<Object>::New(gui_template->NewInstance());
    QSlider * o = new QSlider(Qt::Vertical);

    if( args.Length() > 0) {
        o->setMaximum(args[0]->Int32Value());
    }
    if( args.Length() > 1) {
        o->setMinimum(args[1]->Int32Value());
    }
    if( args.Length() > 2) {
        o->setSingleStep(args[2]->Int32Value());
    }

//    o->setMaximumHeight(80);
    o->setMinimumHeight(160);
    o->setMaximumHeight(160);
    o->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);

    addWidget(o,id);

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>table(const Arguments &args)
{
    HandleScope handle_scope;

    int id = Table_ID;
    Persistent<Object> obj = Persistent<Object>::New(gui_template->NewInstance());
    hTable * o = new hTable();
    o->setSelectionBehavior(QAbstractItemView::SelectRows);
    o->setSelectionMode(QAbstractItemView::SingleSelection);
    o->setAutoScroll(true);
    o->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);

//    o->setColumnCount(2);
//    o->setHorizontalHeaderLabels(QStringList() << "Name" << "Hair Color");

    if( args.Length() > 0) {
        v8::Local<v8::Function> fnc =  v8::Local<v8::Function>::Cast(args[0]);
        o->fnc = v8::Persistent<v8::Function>::New(fnc);
    }
    if( args.Length() > 1) {
        o->foo = args[1]->Int32Value();
    }
    if( args.Length() > 2) {
//        QStringList sl;
        Local<Array> hds = Local<Array>::Cast(args[2]);
//fprintf(stderr,"table length > %d\n", hds->Length());fflush(stderr);
        o->setColumnCount(hds->Length());
        for (unsigned j = 0; j < hds->Length(); j++) {
            Local<Array> hd = Local<Array>::Cast(hds->Get(Integer::New(j)));
//fprintf(stderr,"table length hd > %d\n", hd->Length());fflush(stderr);

            String::Utf8Value nm(hd->Get(Integer::New(0)));
            int32_t sz = (hd->Get(Integer::New(1)))->Int32Value();
            String::Utf8Value ic(hd->Get(Integer::New(2)));
            QString name(*nm);
            save_resource(*ic);

//fprintf(stderr,"table : nm %s sz %d ic %s\n", *nm,sz,*ic);fflush(stderr);

            QTableWidgetItem *it = new QTableWidgetItem(QIcon(*ic),QString(*nm));
//fprintf(stderr,"table : it %d \n",it);fflush(stderr);

            o->setHorizontalHeaderItem(j,it);
            o->setColumnWidth(j,sz);
            if( !name.length()) o->hideColumn(j);
//            sl += QString(*nm);
        }
//        o->setHorizontalHeaderLabels(sl);
    }
    if( args.Length() > 3) {

        Local<Array> acs = Local<Array>::Cast(args[3]);
        for (unsigned j = 0; j < acs->Length(); j++) {
            Local<Object> obj = Local<Object>::Cast(acs->Get(Integer::New(j)));
            Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
            void* oo = field->Value();
            int32_t id = obj->GetInternalField(1)->Int32Value();

            if( id != Action_ID) continue;
//fprintf(stderr,"table action:\n");fflush(stderr);
            o->addAction(( QAction *)oo);
        }
        o->setContextMenuPolicy(Qt::ActionsContextMenu);
    }

    addWidget(o,id);
    MainWindow::connect(o, SIGNAL(currentItemChanged( QTableWidgetItem * , QTableWidgetItem *   )),
            w, SLOT(table_do(QTableWidgetItem * , QTableWidgetItem * )));

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>timer(const Arguments &args)
{
    HandleScope handle_scope;
    int msec = 1000;
    int foo = 777;

    int id = Timer_ID;
    Persistent<Object> obj = Persistent<Object>::New(gui_template->NewInstance());
    hTimer * o = new hTimer();

    if( args.Length() > 0) {
        v8::Local<v8::Function> fnc =  v8::Local<v8::Function>::Cast(args[0]);
        o->fnc = v8::Persistent<v8::Function>::New(fnc);
    }

    if( args.Length() > 1) {
        msec = args[1]->Int32Value();
    }

    if( args.Length() > 2) {
        foo = args[2]->Int32Value();
    }

    o->foo = foo;

    MainWindow::connect(o, SIGNAL(timeout()), w, SLOT(timer_do()));

    o->start(msec);

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>dlg(const Arguments &args)
{
    HandleScope handle_scope;
    int id = Dialog_ID;
    Persistent<Object> obj = Persistent<Object>::New(gui_template->NewInstance());
    QDialog * o = new QDialog();

//    o->setWindowFlags(o->windowFlags() & ~Qt::WindowCloseButtonHint);

    Qt::WindowFlags flags = 0;
    flags = Qt::Window;
    flags |= Qt::CustomizeWindowHint;
    flags |= Qt::WindowTitleHint;
    flags |= Qt::WindowSystemMenuHint;
    flags |= Qt::WindowMinimizeButtonHint;
    o->setWindowFlags(flags);

    if( prg_ic) o->setWindowIcon(*prg_ic);
    else o->setWindowIcon(QIcon(":hjs.gif"));

    if( args.Length() > 0) {
        o->setModal(true);
    }

    QVBoxLayout *lt = new QVBoxLayout;
    lt->setSpacing(0);
//    lt->setSpacing(4);


    o->setLayout(lt);

    obj->SetInternalField(0, External::New(lt));
    obj->SetInternalField(1, Int32::New(id));
    obj->SetInternalField(2, External::New(o));

    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>list(const Arguments &args)
{
    HandleScope handle_scope;

    int id = List_ID;
    Persistent<Object> obj = Persistent<Object>::New(gui_template->NewInstance());
    hList * o = new hList();

    if( args.Length() > 0) {
        v8::Local<v8::Function> fnc =  v8::Local<v8::Function>::Cast(args[0]);
        o->fnc = v8::Persistent<v8::Function>::New(fnc);
    }
    if( args.Length() > 1) {
        o->foo = args[1]->Int32Value();
    }

    if( args.Length() > 2) {
        Local<Array> acs = Local<Array>::Cast(args[2]);
        for (unsigned j = 0; j < acs->Length(); j++) {
            Local<Object> obj = Local<Object>::Cast(acs->Get(Integer::New(j)));
            Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
            void* oo = field->Value();
            int32_t id = obj->GetInternalField(1)->Int32Value();

            if( id != Action_ID) continue;
//fprintf(stderr,"table action:\n");fflush(stderr);
            o->addAction(( QAction *)oo);
        }
        o->setContextMenuPolicy(Qt::ActionsContextMenu);
    }

    addWidget(o,id);
    MainWindow::connect(o, SIGNAL(currentItemChanged( QListWidgetItem * , QListWidgetItem *   )),
            w, SLOT(list_do(QListWidgetItem * , QListWidgetItem *  )));

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}
/*
static v8::Handle<v8::Value>web_ld(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    Handle<Object> obj = args.This();
    Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
    void* o = field->Value();
    int32_t id = obj->GetInternalField(1)->Int32Value();

    if( id != Web_ID) return handle_scope.Close(Boolean::New(false));

    String::Utf8Value zn(args[0]);

    QWebView * oweb = (QWebView * )o;

    oweb->load(QUrl(*zn));

    return handle_scope.Close(Boolean::New(true));
}
*/
/*
static v8::Handle<v8::Value>web_set(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    Handle<Object> obj = args.This();
    Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
    void* o = field->Value();
    int32_t id = obj->GetInternalField(1)->Int32Value();

    if( id != Web_ID) return handle_scope.Close(Boolean::New(false));

    String::Utf8Value zn(args[0]);

    QWebView * oweb = (QWebView * )o;

    oweb->setHtml(*zn);

    return handle_scope.Close(Boolean::New(true));
}
*/
/*
static v8::Handle<v8::Value>web(const Arguments &args)
{
    HandleScope handle_scope;

    int id = Web_ID;
    Persistent<Object> obj = Persistent<Object>::New(gui_template->NewInstance());
    QWebView * o = new QWebView();

    if( args.Length() > 0) {
        int h = args[0]->Int32Value();
        o->setMaximumHeight(h);
    }


//    o->load(QUrl("http:://www.lenta.ru"));

    obj->Set(v8::String::NewSymbol("ld"),v8::FunctionTemplate::New(web_ld)->GetFunction());
    obj->Set(v8::String::NewSymbol("set"),v8::FunctionTemplate::New(web_set)->GetFunction());

    addWidget(o,id);

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}
*/
static v8::Handle<v8::Value>tree(const Arguments &args)
{
    HandleScope handle_scope;

    int id = Tree_ID;
    Persistent<Object> obj = Persistent<Object>::New(gui_template->NewInstance());
    hTree * o = new hTree();

    if( args.Length() > 0) {
        v8::Local<v8::Function> fnc =  v8::Local<v8::Function>::Cast(args[0]);
        o->fnc = v8::Persistent<v8::Function>::New(fnc);
    }
    if( args.Length() > 1) {
        o->foo = args[1]->Int32Value();
    }
    if( args.Length() > 2) {
//        QStringList sl;
        Local<Array> hds = Local<Array>::Cast(args[2]);
//fprintf(stderr,"table length > %d\n", hds->Length());fflush(stderr);
        o->setColumnCount(hds->Length());
        QTreeWidgetItem *it = (QTreeWidgetItem *)0;
        for (unsigned j = 0; j < hds->Length(); j++) {
            Local<Array> hd = Local<Array>::Cast(hds->Get(Integer::New(j)));
//printf("table length hd > %d\n", hd->Length());fflush(stderr);

            String::Utf8Value nm(hd->Get(Integer::New(0)));
            int32_t sz = (hd->Get(Integer::New(1)))->Int32Value();
            String::Utf8Value ic(hd->Get(Integer::New(2)));
            QString name(*nm);
            QString icon_n(*ic);

//printf("table : nm %s sz %d ic %s\n", *nm,sz,*ic);fflush(stderr);

//            QTreeWidgetItem *it = new QTreeWidgetItem(0,QString(*nm));

            if( !it) it = new QTreeWidgetItem();
            else it->addChild(new QTreeWidgetItem());

//            it = new QTreeWidgetItem();

            it->setText(j,name);
            if( icon_n.length()) {
                save_resource(*ic);
                it->setIcon(j,QIcon(icon_n));
            }
//fprintf(stderr,"table : it %d \n",it);fflush(stderr);

//            o->setHeaderItem(it);
            o->setColumnWidth(j,sz);
            if( !name.length()) o->hideColumn(j);
//            sl += QString(*nm);
        }
        o->setHeaderItem(it);

//        o->setHorizontalHeaderLabels(sl);
    }
    if( args.Length() > 3) {

        Local<Array> acs = Local<Array>::Cast(args[3]);
        for (unsigned j = 0; j < acs->Length(); j++) {
            Local<Object> obj = Local<Object>::Cast(acs->Get(Integer::New(j)));
            Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
            void* oo = field->Value();
            int32_t id = obj->GetInternalField(1)->Int32Value();

            if( id != Action_ID) continue;
//fprintf(stderr,"table action:\n");fflush(stderr);
            o->addAction(( QAction *)oo);
        }
        o->setContextMenuPolicy(Qt::ActionsContextMenu);
    }

    addWidget(o,id);
    MainWindow::connect(o, SIGNAL(	currentItemChanged( QTreeWidgetItem * , QTreeWidgetItem *  )),
            w, SLOT(tree_do(QTreeWidgetItem *, QTreeWidgetItem *)));


    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>tab(const Arguments &args)
{
    HandleScope handle_scope;

    int id = Tab_ID;
    Persistent<Object> obj = Persistent<Object>::New(gui_template->NewInstance());
    QTabWidget * o = new QTabWidget();

    if( args.Length() > 0) {
        o->setTabPosition((QTabWidget::TabPosition)args[0]->Int32Value());
    }


    addWidget(o,id);

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>buttons(const Arguments &args)
{
    HandleScope handle_scope;

    int id = ButtonGroup_ID;
    Persistent<Object> obj = Persistent<Object>::New(gui_template->NewInstance());
    QButtonGroup * o = new QButtonGroup();

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>action(const Arguments &args)
{
    HandleScope handle_scope;

    int id = Action_ID;
    Persistent<Object> obj = Persistent<Object>::New(gui_template->NewInstance());

    QString nm;
    QIcon ic;

    hAction * o = new hAction(w);
    if( args.Length() > 0) {
        String::Utf8Value ascii(args[0]);
        nm = *ascii;
        o->setText(rt(*ascii));
//        o->setToolTip("dfg dgsd g");
        o->setStatusTip(*ascii);
    }
    if( args.Length() > 1) {
        v8::Local<v8::Function> fnc =  v8::Local<v8::Function>::Cast(args[1]);
        o->fnc = v8::Persistent<v8::Function>::New(fnc);
    }
    if( args.Length() > 2) {
        String::Utf8Value ascii(args[2]);
        save_resource(*ascii);
        ic = QIcon(*ascii);
        o->setIcon(QIcon(rt(*ascii)));
        o->setIconVisibleInMenu(true);
    }

//    hAction * o = new hAction(ic,nm,w);
    MainWindow::connect(o, SIGNAL(triggered()), w, SLOT(action_do()));

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>check(const Arguments &args)
{
    HandleScope handle_scope;

    int id = Check_ID;
    Persistent<Object> obj = Persistent<Object>::New(gui_template->NewInstance());
    QCheckBox * o = new QCheckBox();
    if( args.Length() > 0) {
        String::Utf8Value ascii(args[0]);
        o->setText(rt(*ascii));
    }
    if( args.Length() > 1) {
        String::Utf8Value ascii(args[1]);
        save_resource(*ascii);
        o->setIcon(QIcon(rt(*ascii)));
    }

    addWidget(o,id);

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>prg_icon(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() > 0) {
        String::Utf8Value ascii(args[0]);
        save_resource(*ascii);
        prg_ic = new QIcon(rt(*ascii));
        w->setWindowIcon(*prg_ic);
    }
    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>radio(const Arguments &args)
{
    HandleScope handle_scope;

    int id = Radio_ID;
    Persistent<Object> obj = Persistent<Object>::New(gui_template->NewInstance());
    QRadioButton * o = new QRadioButton();
    if( args.Length() > 0) {
        String::Utf8Value ascii(args[0]);
        o->setText(rt(*ascii));
    }
    if( args.Length() > 1) {
        String::Utf8Value ascii(args[1]);
        save_resource(*ascii);
        o->setIcon(QIcon(rt(*ascii)));
    }

    if( args.Length() > 2) {
        Local<Object> obj2 = args[2]->ToObject();
        Handle<External> field = Handle<External>::Cast(obj2->GetInternalField(0));
        void* oo = field->Value();
        int32_t id = obj2->GetInternalField(1)->Int32Value();
        if( id == ButtonGroup_ID) {
            QButtonGroup * bg = (QButtonGroup *)oo;
            QList<QAbstractButton *> lb = 	bg->buttons();
            bg->addButton(o,lb.length());
        }
    }

    addWidget(o,id);

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>menus(const Arguments &args)
{
    HandleScope handle_scope;
    if( args.Length() > 0) {
        String::Utf8Value ascii(args[0]);
        QString s = *ascii;
        w->cur_Menu = w->menuBar()->addMenu(s);
    }

    return handle_scope.Close(Boolean::New(true));
}


static v8::Handle<v8::Value>tools(const Arguments &args)
{
    HandleScope handle_scope;
    QString s;

    if( args.Length() > 0) {
        String::Utf8Value ascii(args[0]);
        s = *ascii;
    }
    w->cur_ToolBar = w->addToolBar(s);
    w->addToolBar(Qt::LeftToolBarArea,w->cur_ToolBar);

//    w->cur_ToolBar->setOrientation ( Qt::Vertical );

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>vbox(const Arguments &args)
{
    HandleScope handle_scope;

    int id = Vbox_ID;
    Persistent<Object> obj = Persistent<Object>::New(gui_template->NewInstance());
    QVBoxLayout * o = new QVBoxLayout;

    int sp = GUI_SPACE;
    if( args.Length() > 0) {
        sp = args[0]->Int32Value();
    }
    o->setSpacing(sp);

    addWidget(o,id);

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>hbox(const Arguments &args)
{
    HandleScope handle_scope;

    int id = Hbox_ID;
    Persistent<Object> obj = Persistent<Object>::New(gui_template->NewInstance());
    QHBoxLayout * o = new QHBoxLayout;

    int sp = GUI_SPACE;
    if( args.Length() > 0) {
        sp = args[0]->Int32Value();
    }
    o->setSpacing(sp);

    addWidget(o,id);

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>grid(const Arguments &args)
{
    HandleScope handle_scope;

    int id = Glay_ID;
    Persistent<Object> obj = Persistent<Object>::New(gui_template->NewInstance());
    int c = 2;

    if( args.Length() > 0) {
        int32_t  zn = args[0]->Int32Value();
        c = zn;
    }

    hGreed * o = new hGreed(c);
    o->setSpacing(GUI_SPACE);

//    fprintf(stderr,"grid > %d %d\n",o,id);fflush(stderr);

    addWidget(o,id);

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>def(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() > 0) {
        Local<Object> obj = args[0]->ToObject();
//fprintf(stderr,"def 1\n");fflush(stderr);
        Handle<External> field0 = Handle<External>::Cast(obj->GetInternalField(0));
//fprintf(stderr,"def 2\n");fflush(stderr);
        void* o = field0->Value();
//fprintf(stderr,"def 3\n");fflush(stderr);
        int32_t id = obj->GetInternalField(1)->Int32Value();
//fprintf(stderr,"def 4\n");fflush(stderr);

        if( id == Dialog_ID) id = Vbox_ID;
//fprintf(stderr,"def 5\n");fflush(stderr);

        w->cur_lay = o;
//fprintf(stderr,"def 6\n");fflush(stderr);
        w->cur_lay_type = id;
//fprintf(stderr,"def 7\n");fflush(stderr);
    } else {
        w->cur_lay = w->vl;
        w->cur_lay_type = Vbox_ID;
    }

//fprintf(stderr,"def 99\n");fflush(stderr);
    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>wclose(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() > 0) {
        Local<Object> obj = args[0]->ToObject();
        int32_t id = obj->GetInternalField(1)->Int32Value();
        if( id != Dialog_ID) return handle_scope.Close(Boolean::New(false));

        Handle<External> field2 = Handle<External>::Cast(obj->GetInternalField(2));
        void* o = field2->Value();
        QDialog *dlg = (QDialog *)o;

        dlg->close();

        return handle_scope.Close(Boolean::New(true));
    }

    close_flg = true;

    w->close();

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>getXY(const Arguments &args)
{
    HandleScope handle_scope;

    Handle<Object> obj = Object::New();

    QPoint pt(QCursor::pos ());
//    fprintf(stderr,"x %d y %d\n", pt.x(),pt.y());fflush(stderr);
    obj->Set(v8::String::NewSymbol("x"),Int32::New(pt.x()));
    obj->Set(v8::String::NewSymbol("y"),Int32::New(pt.y()));

    QRect rec = qApp->desktop()->screenGeometry();

    obj->Set(v8::String::NewSymbol("h"),Int32::New(rec.height()));
    obj->Set(v8::String::NewSymbol("w"),Int32::New(rec.width()));

    QPoint pnt = w->pos();
    QSize sz = w->size();

    if( args.Length() > 0) {//dlg
        Local<Object> obj = args[0]->ToObject();
        int32_t id = obj->GetInternalField(1)->Int32Value();

        if( id == Dialog_ID) {
            Handle<External> field2 = Handle<External>::Cast(obj->GetInternalField(2));
            void* o = field2->Value();
            QDialog *dlg = (QDialog *)o;

            pnt = dlg->pos();
            sz = dlg->size();
        }
    }

    obj->Set(v8::String::NewSymbol("wx"),Int32::New(pnt.x()));
    obj->Set(v8::String::NewSymbol("wy"),Int32::New(pnt.y()));
    obj->Set(v8::String::NewSymbol("wh"),Int32::New(sz.height()));
    obj->Set(v8::String::NewSymbol("ww"),Int32::New(sz.width()));

    return handle_scope.Close(obj);
}

static v8::Handle<v8::Value>add(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    Local<Object> obj = args[0]->ToObject();
    Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
    void* o = field->Value();
    int32_t id = obj->GetInternalField(1)->Int32Value();

    if( id == Tab_ID) { // ob,nm,ic
        QWidget *wt = new QWidget;
        QVBoxLayout *lt = new QVBoxLayout;
        wt->setLayout(lt);

        QString nm;
        QString ic;

        if( args.Length() > 1) {
            String::Utf8Value val(args[1]);
            nm = *val;
        }
        if( args.Length() > 2) {
            String::Utf8Value val(args[2]);
            ic = *val;
            save_resource(*val);
        }

        ((QTabWidget *)o)->addTab(wt,QIcon(ic),nm);

        int id = Vbox_ID;
        Persistent<Object> obj = Persistent<Object>::New(gui_template->NewInstance());

        obj->SetInternalField(0, External::New(lt));
        obj->SetInternalField(1, Int32::New(id));
        return handle_scope.Close(obj);
    }

    if( id == Table_ID) { // ob,[[nm,ic,check],[2],[3]...]
        if( args.Length() < 2) return handle_scope.Close(Boolean::New(false));
        hTable *tbl = (hTable *)o;
        int rows = tbl->rowCount();
        tbl->setRowCount(rows + 1);

        Local<Array> hds = Local<Array>::Cast(args[1]);
        for (unsigned j = 0; j < hds->Length(); j++) {
            Local<Array> hd = Local<Array>::Cast(hds->Get(Integer::New(j)));

            String::Utf8Value nm(hd->Get(Integer::New(0)));
            String::Utf8Value ic(hd->Get(Integer::New(1)));
            save_resource(*ic);

            QTableWidgetItem *it = new QTableWidgetItem(QIcon(*ic),QString(*nm));
            it->setToolTip(*nm);
            it->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);

            if( hd->Length() > 2) {
                int32_t check = (hd->Get(Integer::New(2)))->Int32Value();
                it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
                it->setCheckState((Qt::CheckState)check);
            }

            tbl->setItem(rows,j,it);
        }

        return handle_scope.Close(Number::New(rows));
    }

    if( id == Tree_ID) { // ob,[[nm,ic,check],[2],[3]...],root_it
        if( args.Length() < 2) return handle_scope.Close(Boolean::New(false));
        hTree *tbl = (hTree *)o;
        QTreeWidgetItem *root = 0;
        QTreeWidgetItem *it = 0;

        if( args.Length() > 2) {
            Local<Object> oi = args[2]->ToObject();
            Handle<External> f_oi = Handle<External>::Cast(oi->GetInternalField(0));
            void* o_oi = f_oi->Value();
            int32_t id_oi = oi->GetInternalField(1)->Int32Value();
            if( id_oi == TreeItem_ID) root = (QTreeWidgetItem *)o_oi;
        }

        if( !root) it = new QTreeWidgetItem(tbl);
        else it = new QTreeWidgetItem(root);

//        it->setFlags(it->flags() | Qt::ItemIsEditable | Qt::ItemIsSelectable);
        it->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);

        Local<Array> hds = Local<Array>::Cast(args[1]);
        for (unsigned j = 0; j < hds->Length(); j++) {
            Local<Array> hd = Local<Array>::Cast(hds->Get(Integer::New(j)));

            String::Utf8Value nm(hd->Get(Integer::New(0)));
            String::Utf8Value ic(hd->Get(Integer::New(1)));
            QString name(*nm);
            QString icon_n(*ic);

            it->setText(j,name);
            if( icon_n.length()) {
                save_resource(*ic);
                it->setIcon(j,QIcon(icon_n));
            }

            it->setToolTip(j,name);

            if( hd->Length() > 2) {
                int32_t check = (hd->Get(Integer::New(2)))->Int32Value();
                it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
                it->setCheckState(j,(Qt::CheckState)check);
            }

        }

        Persistent<Object> obj = Persistent<Object>::New(gui_template->NewInstance());
        obj->SetInternalField(0, External::New(it));
        obj->SetInternalField(1, Int32::New(TreeItem_ID));
        return handle_scope.Close(obj);
    }

    if( id == List_ID) { // ob,nm,ic,check]
        if( args.Length() < 2) return handle_scope.Close(Boolean::New(false));

        hList *ls = (hList *)o;

        QListWidgetItem * it = new QListWidgetItem(ls);
        String::Utf8Value nm(args[1]);
        it->setFlags(Qt::ItemIsSelectable  | Qt::ItemIsEnabled);

        it->setText(*nm);
        if( args.Length() > 2) {
            String::Utf8Value ic(args[2]);
            save_resource(*ic);
            it->setIcon(QIcon(*ic));
        }
        if( args.Length() > 3) {
            int32_t check = args[3]->Int32Value();
            it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
//fprintf(stderr,"SET Qt::ItemIsUserCheckable %s\n",*nm);fflush(stderr);
            it->setCheckState((Qt::CheckState)check);
        }
    }

    if( id == Combo_ID) { // ob,nm,ic
        if( args.Length() < 2) return handle_scope.Close(Boolean::New(false));
        QComboBox *cb = (QComboBox *)o;

        String::Utf8Value nm(args[1]);
        QString icn;

        if( args.Length() > 2) {
            String::Utf8Value ic(args[2]);
            icn = *ic;
            save_resource(*ic);
        }

        if( icn.length()) {
            cb->addItem(QIcon(icn),*nm);
        } else {
            cb->addItem(*nm);
        }
    }

    if( id == Text_ID) { // ob,txt
        if( args.Length() < 2) return handle_scope.Close(Boolean::New(false));
        QTextEdit *ob = (QTextEdit *)o;

        String::Utf8Value nm(args[1]);

        ob->append(*nm);
    }

    if( id == PText_ID) { // ob,txt
        if( args.Length() < 2) return handle_scope.Close(Boolean::New(false));
        QPlainTextEdit *ob = (QPlainTextEdit *)o;

        String::Utf8Value nm(args[1]);

        ob->appendPlainText(*nm);
    }


    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>rem_tab(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 2) return handle_scope.Close(Boolean::New(false));

    Local<Object> obj = args[0]->ToObject();
    Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
    void* o = field->Value();
    int32_t id = obj->GetInternalField(1)->Int32Value();

    int32_t row = args[1]->Int32Value();

    ((QTabWidget *)o)->removeTab(row);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>hideW(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 2) return handle_scope.Close(Boolean::New(false));

    Local<Object> obj = args[0]->ToObject();
    Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
    void* o = field->Value();
    int32_t id = obj->GetInternalField(1)->Int32Value();

    int32_t row = args[1]->Int32Value();

    if( args.Length() < 3)
        ((QTabWidget *)o)->setTabEnabled(row,false);
    else
        ((QTabWidget *)o)->setTabEnabled(row,true);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>clr(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    Local<Object> obj = args[0]->ToObject();
    Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
    void* o = field->Value();
    int32_t id = obj->GetInternalField(1)->Int32Value();


    if( id == Table_ID) { // ob,
        hTable *tbl = (hTable *)o;
        tbl->clearContents();
        tbl->setRowCount(0);
    }

    if( id == Tree_ID) { // ob
        hTree *tbl = (hTree *)o;
        tbl->clear();
    }

    if( id == List_ID) { // ob
        hList *ls = (hList *)o;
        ls->clear();
    }

    if( id == Combo_ID) { // ob
        QComboBox *cb = (QComboBox *)o;
        cb->clear();
    }

    if( id == Text_ID) { // ob
        QTextEdit *ob = (QTextEdit *)o;
        ob->clear();
    }

    if( id == PText_ID) { // ob
        QPlainTextEdit *ob = (QPlainTextEdit *)o;
        ob->clear();
    }


    return handle_scope.Close(Boolean::New(true));
}

static QTreeWidgetItem * tree_ch(QTreeWidgetItem *it,int idx,int &cur)
{
    HandleScope handle_scope;

    QTreeWidgetItem *nxt =0;

    for(int i = 0; i < it->childCount(); ++i) {
        ++cur;
        nxt = it->child(i);
        if( cur >= idx) return nxt;
        nxt = tree_ch(nxt,idx,cur);
        if( cur >= idx) return nxt;

        nxt= 0;
    }

    return(nxt);
}

static QTreeWidgetItem * tree_idx(hTree *tr,int idx)
{
    int cur = 0;
    QTreeWidgetItem *it=0;

    for( int i = 0; i < tr->topLevelItemCount(); ++i,++cur) {
        it = tr->topLevelItem(i);
        if( cur >= idx) break;
        it = tree_ch(it,idx,cur);
        if( cur >= idx) break;

        it = 0;
    }

    if( cur == idx) return it;

    return 0;
}

static int tree_pos_ch(QTreeWidgetItem *it,QTreeWidgetItem *sit,int &cur)
{
    QTreeWidgetItem *nxt =0;

    for(int i = 0; i < it->childCount(); ++i) {
        ++cur;
        nxt = it->child(i);
        if( sit == nxt) return cur;
        if( tree_pos_ch(nxt,sit,cur) != -1) return cur;

        nxt= 0;
    }

    return(-1);
}

static int tree_pos(hTree *tr)
{
    int cur = 0;
    QTreeWidgetItem *it=0;
    QTreeWidgetItem *sit = tr->currentItem();
    if( !sit) return -1;

    for( int i = 0; i < tr->topLevelItemCount(); ++i,++cur) {
        it = tr->topLevelItem(i);
        if( it == sit) return cur;
        if( tree_pos_ch(it,sit,cur) != -1) return cur;
    }

    return -1;
}


static v8::Handle<v8::Value>pos(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Int32::New(-1));

    Local<Object> obj = args[0]->ToObject();
    Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
    void* o = field->Value();
    int32_t id = obj->GetInternalField(1)->Int32Value();

    switch( id) {
    case Table_ID:{// ob,pos?
        hTable *tbl = (hTable *)o;
        if( args.Length() > 1) {
            int32_t row = args[1]->Int32Value();
//            tbl->setCurrentCell(row,0,QItemSelectionModel::SelectCurrent);
            tbl->setCurrentCell(row,0);
//            QTableWidgetItem * ci = tbl->currentItem ();
            QTableWidgetItem * cii = tbl->currentItem ();
            QTableWidgetItem * ci = tbl->item(row,0);

//            fprintf(stderr"ci %d %d \n",cii,ci);fflush(stderr);

            if( ci) {
//                QModelIndex mx = tbl->indexFromItem( ci);
//                tbl->scrollTo(mx);
//            fprintf(stderr"ciiiii %d %d\n",cii,ci);fflush(stderr);

                tbl->scrollToItem(ci,QAbstractItemView::EnsureVisible);

//                tbl->setCurrentItem(ci);

            }

        }else {
            int pos = tbl->currentRow();
            return handle_scope.Close(Int32::New(pos));
        }

        }break;

    case List_ID:{// ob,pos?
        hList *ob = (hList *)o;
        if( args.Length() > 1) {
            int32_t row = args[1]->Int32Value();
            ob->setCurrentRow(row);
            QListWidgetItem  * ci = ob->currentItem ();
            if( ci) ob->scrollToItem(ci);

        }else {
            int pos = ob->currentRow();
            return handle_scope.Close(Int32::New(pos));
        }

        }break;

    case Tree_ID:{// ob,pos?
        hTree *ob = (hTree *)o;
        if( args.Length() > 1) {
            int32_t row = args[1]->Int32Value();
            QTreeWidgetItem *it = tree_idx(ob,row);
            if( it) {
                ob->setCurrentItem(it);
                ob->scrollToItem(it);

            }
        }else {
            return handle_scope.Close(Int32::New(tree_pos(ob)));
        }

        }break;
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>del(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 2) return handle_scope.Close(Boolean::New(false));

    Local<Object> obj = args[0]->ToObject();
    Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
    void* o = field->Value();
    int32_t id = obj->GetInternalField(1)->Int32Value();
    int32_t row = args[1]->Int32Value();

    switch( id) {
    case Table_ID:{// ob,pos?
        hTable *tbl = (hTable *)o;
        tbl->removeRow(row);
        }break;

    case List_ID:{// ob,pos?
        hList *ob = (hList *)o;
        QListWidgetItem *	it = ob->item (row );
        if( it) delete(it);

        }break;

    case Tree_ID:{// ob,pos?
        hTree *ob = (hTree *)o;
        QTreeWidgetItem *it = tree_idx(ob,row);
        if( it) delete(it);

        }break;
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>set(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    Local<Object> obj = args[0]->ToObject();
    Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
    void* o = field->Value();
    int32_t id = obj->GetInternalField(1)->Int32Value();

/*

Check_ID,Radio_ID,ButtonGroup_ID
,Label_ID,Lcd_ID,Progress_ID,Combo_ID,Line_ID,Text_ID,PText_ID,Spin_ID,Dial_ID,Dttm_ID,Hslider_ID,Vslider_ID
,Tab_ID
,Table_ID,Tree_ID,,List_ID
,
};
*/
    if( args.Length() < 2 && id != Radio_ID) return handle_scope.Close(Boolean::New(false));

//    fprintf(stderr,"set id %d",id);fflush(stderr);

    switch( id) {
    case Check_ID:{// ob,check
        QCheckBox *ob = (QCheckBox *)o;
        int32_t check = args[1]->Int32Value();
        ob->setCheckState((Qt::CheckState) check);
        }break;

    case Radio_ID:{// ob
        QRadioButton *ob = (QRadioButton *)o;
        ob->setChecked(true);
        }break;

    case ButtonGroup_ID:{// ob,indx
        QButtonGroup *ob = (QButtonGroup *)o;
        int32_t indx = args[1]->Int32Value();
        QAbstractButton * bt = ob->button( indx );
        bt->setChecked(true);

        }break;

    case Label_ID:{// ob,txt,ic
        QLabel *ob = (QLabel *)o;
        String::Utf8Value ascii(args[1]);
        ob->setText(*ascii);
        if( args.Length() > 2) {
            String::Utf8Value ascii(args[2]);
            save_resource(*ascii);
            ob->setPixmap(QPixmap(rt(*ascii)));
        }

        }break;

    case Lcd_ID:{
        QLCDNumber *ob = (QLCDNumber *)o;
        int32_t indx = args[1]->Int32Value();
        ob->display(indx);
        }break;

    case Progress_ID:{
        QProgressBar *ob = (QProgressBar *)o;
        int32_t indx = args[1]->Int32Value();
        ob->setValue(indx);
        }break;

    case Combo_ID:{
        QComboBox *ob = (QComboBox *)o;
        String::Utf8Value ascii(args[1]);
        QLineEdit * le = ob->lineEdit ();
        le->setText(*ascii);
        }break;

    case Line_ID:{
        QLineEdit *ob = (QLineEdit *)o;
        String::Utf8Value txt(args[1]);
        ob->setText(*txt);
        }break;

    case Text_ID:{
        QTextEdit *ob = (QTextEdit *)o;
        String::Utf8Value txt(args[1]);
        ob->setText(*txt);
        }break;

    case PText_ID:{
        QPlainTextEdit *ob = (QPlainTextEdit *)o;
        String::Utf8Value txt(args[1]);
        ob->setPlainText(*txt);
        }break;

    case Spin_ID:{
        QSpinBox *ob = (QSpinBox *)o;
        int32_t indx = args[1]->Int32Value();
        ob->setValue(indx);
        }break;

    case Dial_ID:{
        QDial *ob = (QDial *)o;
        int32_t indx = args[1]->Int32Value();
        ob->setValue(indx);
        }break;

    case Dttm_ID:{// ob,yy,mm,dd,hh,min,sek
        QDateTimeEdit *ob = (QDateTimeEdit *)o;
//        int32_t indx = args[1]->Int32Value();
        QDate dt;
        QTime tm;

        int yy = 2000;
        int mm = 1;
        int dd = 1;
        int hh = 0;
        int min = 0;
        int sec = 0;

        if( args.Length() > 1) yy = args[1]->Int32Value();
        if( args.Length() > 2) mm = args[2]->Int32Value();
        if( args.Length() > 3) dd = args[3]->Int32Value();
        if( args.Length() > 4) hh = args[4]->Int32Value();
        if( args.Length() > 5) min = args[5]->Int32Value();
        if( args.Length() > 6) sec = args[6]->Int32Value();

        dt.setDate(yy,mm,dd);
        tm.setHMS(hh,min,sec);

        QDateTime dttm(dt,tm);
        ob->setDateTime(dttm);
        }break;

    case Hslider_ID:
    case Vslider_ID:
        {
        QSlider *ob = (QSlider *)o;
        int32_t indx = args[1]->Int32Value();
        ob->setValue(indx);
        }break;

    case Tab_ID:{// ob,idx
        QTabWidget *ob = (QTabWidget *)o;
        int32_t indx = args[1]->Int32Value();
        ob->setCurrentIndex(indx);
        }break;

    case Table_ID:{// ob,row,[[nm,ic,check],[2],[3]...]
        if( args.Length() < 3) return handle_scope.Close(Boolean::New(false));
        hTable *tbl = (hTable *)o;
        int32_t rows = args[1]->Int32Value();

        Local<Array> hds = Local<Array>::Cast(args[2]);
        for (unsigned j = 0; j < hds->Length(); j++) {
            Local<Array> hd = Local<Array>::Cast(hds->Get(Integer::New(j)));

            String::Utf8Value nm(hd->Get(Integer::New(0)));
            String::Utf8Value ic(hd->Get(Integer::New(1)));
            save_resource(*ic);

            QTableWidgetItem *it = new QTableWidgetItem(QIcon(*ic),QString(*nm));
            it->setToolTip(*nm);

            if( hd->Length() > 2) {
                int32_t check = (hd->Get(Integer::New(2)))->Int32Value();
                it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
                it->setCheckState((Qt::CheckState)check);
            }

            tbl->setItem(rows,j,it);
        }

        }break;

    case Tree_ID:{// ob,idx,[[nm,ic,check],[2],[3]...]
        if( args.Length() < 3) return handle_scope.Close(Boolean::New(false));
        hTree *tr = (hTree *)o;
        int32_t row = args[1]->Int32Value();
/*
        QTreeWidgetItem *it =  tr->topLevelItemCount() ? tr->topLevelItem(0) :0;
        if( !it) return Boolean::New(true);
        for(int i=1; i <= row; ++i) {
//            fprintf(stderr,"i %d\n",i);fflush(stderr);
            it = tr->itemBelow(it);
            if( !it) return Boolean::New(true);

            fprintf(stderr,"i %d\n",i);fflush(stderr);

        }
*/
        QTreeWidgetItem *it = tree_idx(tr,row);
        if( !it) return handle_scope.Close(Boolean::New(false));

        Local<Array> hds = Local<Array>::Cast(args[2]);
        for (unsigned j = 0; j < hds->Length(); j++) {
            Local<Array> hd = Local<Array>::Cast(hds->Get(Integer::New(j)));

            String::Utf8Value nm(hd->Get(Integer::New(0)));
            String::Utf8Value ic(hd->Get(Integer::New(1)));
            QString name(*nm);
            QString icon_n(*ic);

            it->setText(j,name);
            if( icon_n.length()) {
                save_resource(*ic);
                it->setIcon(j,QIcon(icon_n));
            }

            it->setToolTip(j,name);

            if( hd->Length() > 2) {
                int32_t check = (hd->Get(Integer::New(2)))->Int32Value();
                it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
                it->setCheckState(j,(Qt::CheckState)check);
            }

        }
        }break;

    case List_ID:{// ob,idx,nm,ic,check]
        hList *ob = (hList *)o;
        if( args.Length() < 3) return handle_scope.Close(Boolean::New(false));
        int32_t row = args[1]->Int32Value();

        hList *ls = (hList *)o;

//        QListWidgetItem * it = new QListWidgetItem(ls);
        QListWidgetItem * it = ob->item((int)row);

        String::Utf8Value nm(args[2]);
        it->setText(*nm);
        if( args.Length() > 3) {
            String::Utf8Value ic(args[3]);
            save_resource(*ic);
            it->setIcon(QIcon(*ic));
        }
        if( args.Length() > 4) {
            int32_t check = args[4]->Int32Value();
            it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
            it->setCheckState((Qt::CheckState)check);
        }

        }break;
    }

    return handle_scope.Close(Boolean::New(false));
}

static v8::Handle<v8::Value>get(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    Local<Object> obj = args[0]->ToObject();
    Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
    void* o = field->Value();
    int32_t id = obj->GetInternalField(1)->Int32Value();

/*

Check_ID,Radio_ID,ButtonGroup_ID
,Label_ID,Lcd_ID,Progress_ID,Combo_ID,Line_ID,Text_ID,PText_ID,Spin_ID,Dial_ID,Dttm_ID,Hslider_ID,Vslider_ID
,Tab_ID
,Table_ID,Tree_ID,,List_ID
,
};
*/
//    if( args.Length() < 2 && id != Radio_ID) return Boolean::New(true);

//    fprintf(stderr,"set id %d",id);fflush(stderr);

    switch( id) {
    case Check_ID:{// ob,check
        QCheckBox *ob = (QCheckBox *)o;
        return handle_scope.Close(Boolean::New(ob->isChecked()));
        }break;

    case Radio_ID:{// ob
        QRadioButton *ob = (QRadioButton *)o;
        return handle_scope.Close(Boolean::New(ob->isChecked()));
        }break;

    case ButtonGroup_ID:{// ob,indx
        QButtonGroup *ob = (QButtonGroup *)o;
        return handle_scope.Close(Int32::New(ob->checkedId()));
        }break;

    case Combo_ID:{
        QComboBox *ob = (QComboBox *)o;
        QLineEdit * le = ob->lineEdit ();
        QString s = le->text();
        return  handle_scope.Close(String::New(rd(s)));

        }break;

    case Line_ID:{
        QLineEdit *ob = (QLineEdit *)o;
        QString s = ob->text();
        return  handle_scope.Close(String::New(rd(s)));
        }break;

    case Text_ID:{
        QTextEdit *ob = (QTextEdit *)o;
        QString s = ob->toPlainText();
        return  handle_scope.Close(String::New(rd(s)));
        }break;

    case PText_ID:{
        QPlainTextEdit *ob = (QPlainTextEdit *)o;
        QString s = ob->toPlainText();
        return  handle_scope.Close(String::New(rd(s)));
        }break;

    case Spin_ID:{
        QSpinBox *ob = (QSpinBox *)o;
        return handle_scope.Close(Int32::New(ob->value()));
        }break;

    case Dial_ID:{
        QDial *ob = (QDial *)o;
        return handle_scope.Close(Int32::New(ob->value()));
        }break;

    case Dttm_ID:{// ob
        QDateTimeEdit *ob = (QDateTimeEdit *)o;
//        int32_t indx = args[1]->Int32Value();
        QDateTime dttm(ob->dateTime());

        Handle<Object> obj = Object::New();

        obj->Set(v8::String::NewSymbol("y"),Int32::New(dttm.date().year()));
        obj->Set(v8::String::NewSymbol("m"),Int32::New(dttm.date().month()));
        obj->Set(v8::String::NewSymbol("d"),Int32::New(dttm.date().day()));

        obj->Set(v8::String::NewSymbol("h"),Int32::New(dttm.time().hour()));
        obj->Set(v8::String::NewSymbol("mi"),Int32::New(dttm.time().minute()));
        obj->Set(v8::String::NewSymbol("s"),Int32::New(dttm.time().second()));

        return handle_scope.Close(obj);
        }break;

    case Hslider_ID:
    case Vslider_ID:
        {
        QSlider *ob = (QSlider *)o;
        return handle_scope.Close(Int32::New(ob->value()));
        }break;

    //--

    case Table_ID:{// ob,row?
        hTable *ob = (hTable *)o;
        int32_t row = -1;
        if( args.Length() > 1) {
            row = args[1]->Int32Value();
        } else {
            row = ob->currentRow();
        }

        if( row == -1) return handle_scope.Close(Boolean::New(false));

        QTableWidgetItem * it = ob->item((int)row,0);
        if( !it) return handle_scope.Close(Boolean::New(false));


        Handle<Array> ar = Array::New();
        for(int i=0; i < ob->columnCount(); ++i){
            Handle<Object> obj = Object::New();
            QTableWidgetItem * it = ob->item((int)row,i);

            if( it == 0) continue;


            obj->Set(v8::String::NewSymbol("v"),String::New(rd(it->text())));
            if( it->flags() & Qt::ItemIsUserCheckable) {
                obj->Set(v8::String::NewSymbol("c"),Int32::New(it->checkState()));
            }

            ar->Set(v8::Number::New(i),obj);
        }

        return handle_scope.Close(ar);

        }break;

    case Tree_ID:{// ob,row?
        hTree *ob = (hTree *)o;
        int32_t row = -1;
        if( args.Length() > 1) {
            row = args[1]->Int32Value();
        } else {
            row = tree_pos(ob);
        }

        if( row == -1) return handle_scope.Close(Boolean::New(false));

        QTreeWidgetItem * it = tree_idx(ob,row);
        if( !it) return handle_scope.Close(Boolean::New(false));

        Handle<Array> ar = Array::New();
        for(int i=0; i < it->columnCount(); ++i){
            Handle<Object> obj = Object::New();
//            QTableWidgetItem * it = ob->item((int)row,i);

            obj->Set(v8::String::NewSymbol("v"),String::New(rd(it->text(i))));
            if( it->flags() & Qt::ItemIsUserCheckable) {
                obj->Set(v8::String::NewSymbol("c"),Int32::New(it->checkState(i)));
            }

            ar->Set(v8::Number::New(i),obj);
        }

        return handle_scope.Close(ar);
        }break;

    case List_ID:{// ob,row?
        hList *ob = (hList *)o;
        int32_t row = -1;
        if( args.Length() > 1) {
            row = args[1]->Int32Value();
        } else {
            row = ob->currentRow();
        }

        if( row == -1) return handle_scope.Close(Boolean::New(false));

        QListWidgetItem * it = ob->item((int)row);
        if( !it) return handle_scope.Close(Boolean::New(false));

        Handle<Object> obj = Object::New();

        obj->Set(v8::String::NewSymbol("v"),String::New(rd(it->text())));
        if( it->flags() & Qt::ItemIsUserCheckable) {
            obj->Set(v8::String::NewSymbol("c"),Int32::New(it->checkState()));
        }

        return handle_scope.Close(obj);
        }break;
    }

    return handle_scope.Close(Boolean::New(false));
}

static v8::Handle<v8::Value>menu(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() <= 0 || !w->cur_Menu) {
        return handle_scope.Close(Boolean::New(false));
    }

    Local<Object> obj = args[0]->ToObject();
    Handle<External> field0 = Handle<External>::Cast(obj->GetInternalField(0));
    void* o = field0->Value();
    int32_t id = obj->GetInternalField(1)->Int32Value();

    if( id != Action_ID) {
        return handle_scope.Close(Boolean::New(false));
    }

    w->cur_Menu->addAction((hAction*)o);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>tool(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() <= 0) {
        return handle_scope.Close(Boolean::New(false));
    }

    Local<Object> obj = args[0]->ToObject();
    Handle<External> field0 = Handle<External>::Cast(obj->GetInternalField(0));
    void* o = field0->Value();
    int32_t id = obj->GetInternalField(1)->Int32Value();

    if( id != Action_ID) {
        return handle_scope.Close(Boolean::New(false));
    }

    w->cur_ToolBar->addAction((hAction*)o);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>mes(const Arguments &args)
{
    HandleScope handle_scope;
    int ret = 1;
    QMessageBox::StandardButton bt;

    if( args.Length() < 3) {
        return handle_scope.Close(Boolean::New(false));
    }

    int32_t tp = args[0]->Int32Value();
    String::Utf8Value zag(args[1]);
    String::Utf8Value txt(args[2]);

    switch(tp) {
    case 1:
        bt = QMessageBox::information(w,*zag,*txt,QMessageBox::Close);
        break;

    case 2:
        bt = QMessageBox::question(w,*zag,*txt,QMessageBox::Ok | QMessageBox::Cancel);
        break;

    case 3:
        bt = QMessageBox::critical(w,*zag,*txt,QMessageBox::Close);
        break;

    case 4:
        QMessageBox::about(w,QString(*zag),QString(*txt));
        break;

    case 0:
    default:
        bt = QMessageBox::warning(w,*zag,*txt,QMessageBox::Close);
        break;
    }

    if( bt != QMessageBox::Ok) ret = 0;

    return handle_scope.Close(Boolean::New(ret));
}

static v8::Handle<v8::Value>file(const Arguments &args)
{
    HandleScope handle_scope;
    int ret = 1;
    QMessageBox::StandardButton bt;
    QString fileName;

    if( args.Length() < 3) {
        return handle_scope.Close(Boolean::New(false));
    }

    int32_t tp = args[0]->Int32Value();
    String::Utf8Value zag(args[1]);
    String::Utf8Value path(args[2]);
    String::Utf8Value ext(args[3]);

    switch(tp) {
    case 1:
        fileName = QFileDialog::getSaveFileName(w,*zag,*path,*ext);
        break;

    case 0:
    default:
        QString exts(*ext);
        if( exts.contains("dir",Qt::CaseInsensitive)){
//            fileName = QFileDialog::getOpenFileName(w,*zag,*path,*ext,0,QFileDialog::ShowDirsOnly);
            fileName = QFileDialog::getExistingDirectory (w,*zag,*path);
        }else{
            fileName = QFileDialog::getOpenFileName(w,*zag,*path,*ext);
        }
        break;
    }

    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QByteArray ba = codec->fromUnicode(fileName);

    return handle_scope.Close(String::New(ba.data()));
}


static v8::Handle<v8::Value>show(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() > 5) {
        Local<Object> obj = args[5]->ToObject();
        int32_t id = obj->GetInternalField(1)->Int32Value();

        if( id != Dialog_ID) {return handle_scope.Close(Boolean::New(false));}

        Handle<External> field2 = Handle<External>::Cast(obj->GetInternalField(2));
        void* o = field2->Value();
        QDialog *dlg = (QDialog *)o;

        String::Utf8Value ascii(args[0]);
        QString s = *ascii;
//        dlg->setWindowTitle("hjs: " + s);
        dlg->setWindowTitle(s);

        QPoint pos;
        QSize size;

        if( args.Length() > 1) {
            size.setWidth(args[1]->Int32Value());
        }
        if( args.Length() > 2) {
            size.setHeight(args[2]->Int32Value());
        }
        if( args.Length() > 3) {
            pos.setX(args[3]->Int32Value());
        }
        if( args.Length() > 4) {
            pos.setY(args[4]->Int32Value());
        }

        if( size.width() && size.height()) {
            dlg->resize(size);
        }
        if( pos.x() && pos.y()) {
            dlg->move(pos);
        }

        dlg->show();

        return handle_scope.Close(Boolean::New(true));
    }

    //--
    if( args.Length() > 0) {
        String::Utf8Value ascii(args[0]);
        QString s = *ascii;
        w->setWindowTitle("hjs: " + s);
    }

    QPoint pos;
    QSize size;

    if( args.Length() > 1) {
        size.setWidth(args[1]->Int32Value());
    }
    if( args.Length() > 2) {
        size.setHeight(args[2]->Int32Value());
    }
    if( args.Length() > 3) {
        pos.setX(args[3]->Int32Value());
    }
    if( args.Length() > 4) {
        pos.setY(args[4]->Int32Value());
    }

    if( size.width() && size.height()) {
        w->resize(size);
    }
    if( pos.x() && pos.y()) {
        w->move(pos);
    }


/*
    if( args.Length() > 0) {
//        sp = args[0]->Int32Value();
    }

    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(600, 400)).toSize();
    move(pos);
    resize(size);
*/

    w->show();

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>space(const Arguments &args)
{
    HandleScope handle_scope;

    switch(w->cur_lay_type) {

    case Vbox_ID:
    case Hbox_ID:
        ((QBoxLayout *)w->cur_lay)->addStretch();
        break;
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>gnext(const Arguments &args)
{
    HandleScope handle_scope;
    switch(w->cur_lay_type) {

    case Glay_ID:
        ((hGreed *)w->cur_lay)->col_cnt++;
        break;
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>menu_sep(const Arguments &args)
{
    HandleScope handle_scope;
    if( w->cur_Menu) {
        w->cur_Menu->addSeparator();
    }

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>tool_sep(const Arguments &args)
{
    HandleScope handle_scope;
    w->cur_ToolBar->addSeparator();

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>sep(const Arguments &args)
{
    HandleScope handle_scope;

//    w->cur_ToolBar->addSeparator();
    QLabel *f = new QLabel();
    f->setFrameStyle(QFrame::HLine);

    switch(w->cur_lay_type) {

    case Vbox_ID:
        f->setFrameStyle(QFrame::HLine);

        if( args.Length() > 0) {
            String::Utf8Value nm(args[0]);
            QLabel *ll = new QLabel(*nm);
            f->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
            ll->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
            QHBoxLayout * h = new QHBoxLayout;
//            h->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
            QLabel *f2 = new QLabel();
            f2->setFrameStyle(QFrame::HLine);
            f2->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);

            h->addWidget(f);
            h->addWidget(ll);
            h->addWidget(f2);

            ((QBoxLayout *)w->cur_lay)->addLayout(h);

            return handle_scope.Close(Boolean::New(true));
        }

        break;
    case Hbox_ID:
        f->setFrameStyle(QFrame::VLine);
        break;
    }

    ((QBoxLayout *)w->cur_lay)->addWidget(f);

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>addWidgetFnc(const Arguments &args)
{
    HandleScope handle_scope;

//    addWidget(o,id);
    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));


    Local<Object> obj = args[0]->ToObject();
    Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
    QWidget* o = (QWidget* )field->Value();

    addWidget(o,EXTERNAL_ID);

    return handle_scope.Close(Boolean::New(true));
}

//---
static v8::Handle<v8::Value>web_ld(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    Handle<Object> obj = args.This();
    Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
    void* o = field->Value();
    int32_t id = obj->GetInternalField(1)->Int32Value();

    if( id != Web_ID) return handle_scope.Close(Boolean::New(false));

    String::Utf8Value zn(args[0]);

    QWebView * oweb = (QWebView * )o;

    oweb->load(QUrl(*zn));

    return handle_scope.Close(Boolean::New(true));
}

static v8::Handle<v8::Value>web_set(const Arguments &args)
{
    HandleScope handle_scope;

    if( args.Length() < 1) return handle_scope.Close(Boolean::New(false));

    Handle<Object> obj = args.This();
    Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
    void* o = field->Value();
    int32_t id = obj->GetInternalField(1)->Int32Value();

    if( id != Web_ID) return handle_scope.Close(Boolean::New(false));

    String::Utf8Value zn(args[0]);

    QWebView * oweb = (QWebView * )o;

    oweb->setHtml(*zn);

    return handle_scope.Close(Boolean::New(true));
}


static v8::Handle<v8::Value>web(const Arguments &args)
{
    HandleScope handle_scope;

    int id = Web_ID;
    Persistent<Object> obj = Persistent<Object>::New(gui_template->NewInstance());
    QWebView * o = new QWebView();

    if( args.Length() > 0) {
        int h = args[0]->Int32Value();
        o->setMaximumHeight(h);
    }


//    o->load(QUrl("http:://www.lenta.ru"));

    obj->Set(v8::String::NewSymbol("ld"),v8::FunctionTemplate::New(web_ld)->GetFunction());
    obj->Set(v8::String::NewSymbol("set"),v8::FunctionTemplate::New(web_set)->GetFunction());

    addWidget(o,id);

    obj->SetInternalField(0, External::New(o));
    obj->SetInternalField(1, Int32::New(id));
    return handle_scope.Close(obj);
}

gui::gui(QObject *parent): QObject(parent)
{
    //--

    Handle<Object> global = context->Global();
    w->cur_lay = w->vl;
    w->cur_lay_type = Vbox_ID;

    gui_template = (Persistent<v8::ObjectTemplate>)v8::ObjectTemplate::New();
    gui_template->SetInternalFieldCount(3);

    Local<Object> ob = Object::New();

    ob->Set(v8::String::NewSymbol("icon"),v8::FunctionTemplate::New(prg_icon)->GetFunction()); // icon
    ob->Set(v8::String::NewSymbol("show"),v8::FunctionTemplate::New(show)->GetFunction()); // name,w,h,x,y,dlg
    ob->Set(v8::String::NewSymbol("button"),v8::FunctionTemplate::New(button)->GetFunction()); //name,fnc,icon,tip,foo,Expanding?
    ob->Set(v8::String::NewSymbol("btn"),v8::FunctionTemplate::New(button)->GetFunction()); //name,fnc,icon,tip,foo
    ob->Set(v8::String::NewSymbol("space"),v8::FunctionTemplate::New(space)->GetFunction());
    ob->Set(v8::String::NewSymbol("sp"),v8::FunctionTemplate::New(space)->GetFunction());
    ob->Set(v8::String::NewSymbol("vb"),v8::FunctionTemplate::New(vbox)->GetFunction()); // sp
    ob->Set(v8::String::NewSymbol("vbox"),v8::FunctionTemplate::New(vbox)->GetFunction()); // sp
    ob->Set(v8::String::NewSymbol("hbox"),v8::FunctionTemplate::New(hbox)->GetFunction()); // sp
    ob->Set(v8::String::NewSymbol("hb"),v8::FunctionTemplate::New(hbox)->GetFunction()); // sp
    ob->Set(v8::String::NewSymbol("grid"),v8::FunctionTemplate::New(grid)->GetFunction()); // col
    ob->Set(v8::String::NewSymbol("gr"),v8::FunctionTemplate::New(grid)->GetFunction()); // col
    ob->Set(v8::String::NewSymbol("def"),v8::FunctionTemplate::New(def)->GetFunction()); // layobj
    ob->Set(v8::String::NewSymbol("action"),v8::FunctionTemplate::New(action)->GetFunction()); //name,fnc,icon,foo
    ob->Set(v8::String::NewSymbol("act"),v8::FunctionTemplate::New(action)->GetFunction()); //name,fnc,icon,foo
    ob->Set(v8::String::NewSymbol("menus"),v8::FunctionTemplate::New(menus)->GetFunction());
    ob->Set(v8::String::NewSymbol("menu"),v8::FunctionTemplate::New(menu)->GetFunction()); // action
    ob->Set(v8::String::NewSymbol("tools"),v8::FunctionTemplate::New(tools)->GetFunction());
    ob->Set(v8::String::NewSymbol("tool"),v8::FunctionTemplate::New(tool)->GetFunction()); // action
    ob->Set(v8::String::NewSymbol("menu_sep"),v8::FunctionTemplate::New(menu_sep)->GetFunction());
    ob->Set(v8::String::NewSymbol("tool_sep"),v8::FunctionTemplate::New(tool_sep)->GetFunction());
    ob->Set(v8::String::NewSymbol("sep"),v8::FunctionTemplate::New(sep)->GetFunction()); // txt
    ob->Set(v8::String::NewSymbol("check"),v8::FunctionTemplate::New(check)->GetFunction()); // name,icon
    ob->Set(v8::String::NewSymbol("buttons"),v8::FunctionTemplate::New(buttons)->GetFunction());
    ob->Set(v8::String::NewSymbol("radio"),v8::FunctionTemplate::New(radio)->GetFunction());  // name,icon,buttons
    ob->Set(v8::String::NewSymbol("lab"),v8::FunctionTemplate::New(lab)->GetFunction()); // name,icon,xExp,yExp
    ob->Set(v8::String::NewSymbol("lcd"),v8::FunctionTemplate::New(lcd)->GetFunction()); // digcnt,val
    ob->Set(v8::String::NewSymbol("prog"),v8::FunctionTemplate::New(prog)->GetFunction()); // val

    ob->Set(v8::String::NewSymbol("combo"),v8::FunctionTemplate::New(combo)->GetFunction());//tip?
    ob->Set(v8::String::NewSymbol("line"),v8::FunctionTemplate::New(line)->GetFunction());//tip?
    ob->Set(v8::String::NewSymbol("text"),v8::FunctionTemplate::New(text)->GetFunction());//tip?
    ob->Set(v8::String::NewSymbol("txt"),v8::FunctionTemplate::New(text)->GetFunction());//tip?
    ob->Set(v8::String::NewSymbol("ptext"),v8::FunctionTemplate::New(ptext)->GetFunction());//tip?
    ob->Set(v8::String::NewSymbol("ptxt"),v8::FunctionTemplate::New(ptext)->GetFunction());//tip?
    ob->Set(v8::String::NewSymbol("spin"),v8::FunctionTemplate::New(spin)->GetFunction()); // max,min,step
    ob->Set(v8::String::NewSymbol("dial"),v8::FunctionTemplate::New(dial)->GetFunction()); // max,min,step
    ob->Set(v8::String::NewSymbol("dttm"),v8::FunctionTemplate::New(dttm)->GetFunction());
    ob->Set(v8::String::NewSymbol("hslider"),v8::FunctionTemplate::New(hslider)->GetFunction());// max,min,step
    ob->Set(v8::String::NewSymbol("vslider"),v8::FunctionTemplate::New(vslider)->GetFunction());// max,min,step

    ob->Set(v8::String::NewSymbol("tab"),v8::FunctionTemplate::New(tab)->GetFunction());// orientation (0|1|2|3)

    ob->Set(v8::String::NewSymbol("table"),v8::FunctionTemplate::New(table)->GetFunction());// fnc,foo,[[nm,size,ic]...],[action...]
    ob->Set(v8::String::NewSymbol("tree"),v8::FunctionTemplate::New(tree)->GetFunction());// fnc,foo,[[nm,size,ic]...],[action...]
    ob->Set(v8::String::NewSymbol("list"),v8::FunctionTemplate::New(list)->GetFunction());// fnc,foo,[action...]

    ob->Set(v8::String::NewSymbol("timer"),v8::FunctionTemplate::New(timer)->GetFunction());// fnc,msec,foo,

    ob->Set(v8::String::NewSymbol("dlg"),v8::FunctionTemplate::New(dlg)->GetFunction());// modal?
    ob->Set(v8::String::NewSymbol("mes"),v8::FunctionTemplate::New(mes)->GetFunction());// 0-4,title,context
    ob->Set(v8::String::NewSymbol("file"),v8::FunctionTemplate::New(file)->GetFunction());// 0-1,zag,path,ext

    ob->Set(v8::String::NewSymbol("web"),v8::FunctionTemplate::New(web)->GetFunction());//

    ob->Set(v8::String::NewSymbol("close"),v8::FunctionTemplate::New(wclose)->GetFunction());// dlg?
    ob->Set(v8::String::NewSymbol("getXY"),v8::FunctionTemplate::New(getXY)->GetFunction());// dlg?
    ob->Set(v8::String::NewSymbol("add"),v8::FunctionTemplate::New(add)->GetFunction());
    ob->Set(v8::String::NewSymbol("clr"),v8::FunctionTemplate::New(clr)->GetFunction());
    ob->Set(v8::String::NewSymbol("set"),v8::FunctionTemplate::New(set)->GetFunction());
    ob->Set(v8::String::NewSymbol("pos"),v8::FunctionTemplate::New(pos)->GetFunction());
    ob->Set(v8::String::NewSymbol("del"),v8::FunctionTemplate::New(del)->GetFunction());
    ob->Set(v8::String::NewSymbol("get"),v8::FunctionTemplate::New(get)->GetFunction());
    ob->Set(v8::String::NewSymbol("disT"),v8::FunctionTemplate::New(hideW)->GetFunction());
    ob->Set(v8::String::NewSymbol("remT"),v8::FunctionTemplate::New(rem_tab)->GetFunction());


    ob->Set(v8::String::NewSymbol("addWidget"),v8::FunctionTemplate::New(addWidgetFnc)->GetFunction());//obj

    //--
    global->Set(String::New("gui"), ob);
    global->Set(String::New("ui"), ob);
}
