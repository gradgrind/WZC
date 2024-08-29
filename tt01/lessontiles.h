#ifndef LESSONTILES_H
#define LESSONTILES_H
#include "fetdata.h"

struct TileFraction {
    int offset;
    int fraction;
    int total;
};

void class_divisions(FetInfo &fet_info);

QMap<int, QList<TileFraction>> course_divisions(
    FetInfo &fet_info, QJsonArray groups);

#endif // LESSONTILES_H
