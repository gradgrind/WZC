#ifndef LESSONTILES_H
#define LESSONTILES_H
#include "database.h"

struct TileFraction {
    int offset;
    int fraction;
    int total;
    QStringList groups;
};

void class_divisions(DBData &db_data);

QMap<int, QList<TileFraction>> course_divisions(
    DBData &db_data, QJsonArray groups);

#endif // LESSONTILES_H
