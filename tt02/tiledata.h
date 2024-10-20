#ifndef TILEDATA_H
#define TILEDATA_H

#include "timetabledata.h"
#include <qjsonobject.h>

class TileData : public QJsonObject
{
public:
    TileData(TimetableData *tt_data, int lesson_id);

    QString teachers_string() { return teachers.join(","); }

    int duration;
    QString subject;
    QStringList teachers;
    QStringList rooms;

    int day;
    int hour;
};

#endif // TILEDATA_H
