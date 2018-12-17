#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include<zlib.h>
#include<time.h>

//#include "wx.h"

#include <v8.h>
//using namespace v8;

//#include "opencv2/objdetect/objdetect.hpp"
//#include "opencv2/imgproc/imgproc.hpp"
//#include "opencv2/highgui/highgui.hpp"
//using namespace std;
//using namespace cv;

#include <QtGui>
#include <QMainWindow>
#include <QtWebKit>
#include <QtSql>
#include <QCryptographicHash>
#include "hhprocess.h"
#include "semaphore.h"
#include "fcntl.h"
#include "stdio.h"


char * get_file_text_utf8(char * fn);

namespace Ui {
class MainWindow;
}

class StdinThread_T : public QThread
{
#define BUF_INT_SIZE 50000
public:
    char	b[BUF_INT_SIZE];
    char	b_in[BUF_INT_SIZE];
    int bin_flg;

    StdinThread_T(QObject *parent = NULL) : QThread(parent)
    {
        bin_flg = 0;
    }

    void run()
    {
        while( gets(b)) {
            while( bin_flg)msleep(5);

            strcpy(b_in,b);
            bin_flg = 1;
//            fprintf(stderr,"%s\n",b);fflush(stderr);
        }
    };
};


class InfinedThread_T : public QThread
{
public:
    time_t tm;
    time_t tm_clone;
    char par[2000];

    InfinedThread_T(QObject *parent = NULL) : QThread(parent)
    {
        tm = 0;
        tm_clone = 0;
        par[0] = 0;
    }

    void set(int sc,char *p)
    {
        if( p == 0) {
            par[0] = 0;
        }else if( p[0] == 0){
            par[0] = 0;
        }else {
            strcpy(par,"-R");
            strcat(par,p);
        }

        if( sc == 0) {
            tm = 0;
        }else{
            tm = time(0) + sc;
        }
    }
    void run();

};

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    QVBoxLayout *vl;

    void *cur_lay;
    int cur_lay_type;
    QMenu *cur_Menu;
    QToolBar *cur_ToolBar;

    Ui::MainWindow *ui;

    StdinThread_T in_t;
    InfinedThread_T infined_t;

public slots:
    void action_do();
    void button_do();

    void tree_do(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void table_do(QTableWidgetItem * current, QTableWidgetItem * previous );
    void list_do(QListWidgetItem * current, QListWidgetItem * previous );

    void timer_do();

private:

};

#endif // MAINWINDOW_H
