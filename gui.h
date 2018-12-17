#ifndef GUI_H
#define GUI_H

#include "mainwindow.h"
#include <QObject>

class hButton : public QPushButton
{
    Q_OBJECT
public:

    int id;
    int foo;

    v8::Persistent<v8::Function> fnc;

    hButton( QString ext);
    bool event( QEvent * e );

signals:
public slots:
};

class hTable : public QTableWidget
{
    Q_OBJECT
public:

    int id;
    int foo;

    v8::Persistent<v8::Function> fnc;
    hTable();

signals:
public slots:
};

class hTree : public QTreeWidget
{
    Q_OBJECT
public:

    int id;
    int foo;

    v8::Persistent<v8::Function> fnc;

    hTree();

signals:
public slots:
};

class hList : public QListWidget
{
    Q_OBJECT
public:

    int id;
    int foo;

    v8::Persistent<v8::Function> fnc;

    hList();

signals:
public slots:
};

class hTimer : public QTimer
{
    Q_OBJECT
public:

    int id;
    int foo;

    v8::Persistent<v8::Function> fnc;

    hTimer();

signals:
public slots:
};


class hAction : public QAction
{
    Q_OBJECT
public:

    int id;
    int foo;
    v8::Persistent<v8::Function> fnc;

    hAction(QObject * parent);
    hAction( QString ext,QObject * parent);
    hAction( const QIcon  icon,QString text,QObject * parent);

protected:
    bool event( QEvent * e );

signals:
public slots:
    void trigger();

};

class hGreed : public QGridLayout
{
    Q_OBJECT
public:
    hGreed( int c = 2);
    int col;
    int col_cnt;

signals:
public slots:
};


class gui : public QObject
{
    Q_OBJECT
public:
    explicit gui(QObject *parent = 0);
    
signals:
    
public slots:
    
};




#endif // GUI_H
