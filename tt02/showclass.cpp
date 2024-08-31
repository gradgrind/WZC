#include "showclass.h"
#include <QJsonArray>

ShowClass::ShowClass(TT_Grid *grid, DBData *db_data, int class_id)
{
    for (int course_id : db_data->class_courses[class_id]) {
        auto course = db_data->Nodes.value(course_id).DATA;
        QStringList teachers;
        auto tlist = course.value("TEACHERS").toArray();
        for (const auto & t : tlist) {
            teachers.append(db_data->get_tag(t.toInt()));
        }
        QString teacher = teachers.join(",");
        QString subject = db_data->get_tag(course.value("SUBJECT").toInt());
        // The rooms need to be done on a lesson basis!
        auto tile_info = db_data->course_tileinfo[course_id];
        auto tiles = tile_info.value(class_id);
        for (int lid : db_data->course_lessons.value(course_id)) {
            auto ldata = db_data->Nodes.value(lid).DATA;
            int len = ldata.value("LENGTH").toInt();
            QStringList rooms;
            auto rlist = ldata.value("ROOMS").toArray();
            for (const auto &r : rlist) {
                rooms.append(db_data->get_tag(r.toInt()));
            }
            QString room = rooms.join(",");
            int d = db_data->days.value(ldata.value("DAY").toInt());
            int h = db_data->hours.value(ldata.value("HOUR").toInt());
            for (const auto &tf : tiles) {
                Tile *t = new Tile(grid,
                    QJsonObject {
                        {"TEXT", subject},
                        {"TL", teacher},
                        {"TR", tf.groups.join(",")},
                        {"BR", room},
                        {"LENGTH", len},
                        {"DIV0", tf.offset},
                        {"DIVS", tf.fraction},
                        {"NDIVS", tf.total},
                    },
                    lid
                );
                grid->place_tile(t, d, h);
            }
        }
    }
}
