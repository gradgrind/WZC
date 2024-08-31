#ifndef LESSONTILES_H
#define LESSONTILES_H
#include "database.h"

void class_divisions(DBData *db_data);

QMap<int, QList<TileFraction>> course_divisions(
    DBData *db_data, QJsonArray groups);

#endif // LESSONTILES_H
