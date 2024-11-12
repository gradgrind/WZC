#include "tiledata.h"
#include <qjsonarray.h>

TileData::TileData(TimetableData *tt_data, int lesson_id)
    : QJsonObject()
{
    DBData *db_data = tt_data->db_data;
    const auto lesson = db_data->Nodes.value(lesson_id);
    const int course_id = lesson["COURSE"].toInt();
    const auto course = db_data->Nodes.value(course_id);
    // Teachers
    const auto tlist = course.value("TEACHERS").toArray();
    for (const auto &t : tlist) {
        teachers.append(db_data->get_tag(t.toInt()));
    }
    // Subject (a single one, no list is supported)
    subject = db_data->get_tag(course.value("SUBJECT").toInt());
    // Rooms. Distinguish the flexible room, if there is one.
    const auto rlist = course.value("FIXED_ROOMS").toArray();
    for (const auto &r : rlist) {
        rooms.append(db_data->get_tag(r.toInt()));
    }
    auto fr{lesson.value("FLEXIBLE_ROOM")};
    if (fr.isUndefined()) {
        if (course.contains("ROOM_CHOICE"))
            rooms.append("(?)");
    } else
        rooms.append(QString("(%1)").arg(db_data->get_tag(fr.toInt())));
    // Lesson length (number of slots)
    duration = lesson.value("LENGTH").toInt();

    //TODO: Does the position (day + hour) really belong here?
    int d0 = lesson.value("DAY").toInt();
    if (d0 == 0) {
        day = -1;
    } else {
        day = db_data->days.value(d0);
        hour = db_data->hours.value(lesson.value("HOUR").toInt());
    }

    //TODO??
    //    const auto tile_info = tt_data->course_tileinfo[course_id];
    //    const auto tiles = tile_info.value(class_id);

    //TODO ...
    /*
    const auto tile_info = tt_data->course_tileinfo[course_id];
    const auto tiles = tile_info.value(class_id);
    for (int lid : db_data->course_lessons.value(course_id)) {
        const auto ldata = db_data->Nodes.value(lid);
        int d0 = ldata.value("DAY").toInt();
        if (d0 == 0) {
            //TODO: Collect unplaced lessons

        } else {
            int d = db_data->days.value(d0);
            int h = db_data->hours.value(ldata.value("HOUR").toInt());
            for (const auto &tf : tiles) {
                Tile *t = new Tile(grid,
                                   QJsonObject{
                                       {"TEXT", subject},
                                       {"TL", teacher},
                                       {"TR", tf.groups.join(",")},
                                       {"BR", roomlist.join(",")},
                                       {"LENGTH", len},
                                       {"DIV0", tf.offset},
                                       {"DIVS", tf.fraction},
                                       {"NDIVS", tf.total},
                                   },
                                   lid);
                grid->place_tile(t, d, h);
            }
        }
    }
*/
}
