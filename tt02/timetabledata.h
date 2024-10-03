#ifndef TIMETABLEDATA_H
#define TIMETABLEDATA_H
#include "database.h"

class TimetableData
{
public:
    TimetableData(DBData *dbdata);

    DBData *db_data; // not owned here
    QHash<int, class_divs> class_subgroup_divisions;
    // The tiles are divided only for the class view. The map below
    // supplies a list of tiles for each involved class.
    QHash<int, QMap<int, QList<TileFraction>>> course_tileinfo;
    QHash<int, QList<int>> teacher_courses;
    QHash<int, QList<int>> class_courses;

private:
    QHash<int, class_divs> class_divisions();
    QMap<int, QList<TileFraction>> course_divisions(const QJsonArray groups);
};

#endif // TIMETABLEDATA_H
