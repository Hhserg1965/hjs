#ifndef QPARSEJS_H
#define QPARSEJS_H

#include "bb.h"
using namespace v8;

typedef enum {
    QJS_UNDEFINED = 0,
    QJS_OBJECT = 1,
    QJS_ARRAY = 2,
    QJS_STRING = 3,
    QJS_PRIMITIVE = 4
} Qjstype_t;





#endif // QPARSEJS_H
