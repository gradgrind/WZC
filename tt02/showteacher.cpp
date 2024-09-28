#include "showteacher.h"
#include <QJsonArray>

ShowTeacher::ShowTeacher(TT_Grid *grid, DBData *db_data, int teacher_id)
{
    for (int course_id : db_data->teacher_courses[teacher_id]) {
        auto course = db_data->Nodes.value(course_id);
        QStringList groups;
        auto glist = course.value("STUDENTS").toArray();
        for (const auto & g : glist) {
            // Combine class and group
            auto node = db_data->Nodes.value(g.toInt());
            auto gtag = node.value("TAG").toString();
            auto ctag = db_data->get_tag(node.value("CLASS").toInt());
            if (gtag.isEmpty()) {
                groups.append(ctag);
            } else {
                groups.append(ctag + "." + gtag);
            }
        }
        QString group = groups.join(",");
        QString subject = db_data->get_tag(course.value("SUBJECT").toInt());
        // The rooms need to be done on a lesson basis!
        for (int lid : db_data->course_lessons.value(course_id)) {
            auto ldata = db_data->Nodes.value(lid);
            int len = ldata.value("LENGTH").toInt();
            QStringList rooms;
            auto rlist = ldata.value("ROOMS").toArray();
            for (const auto &r : rlist) {
                rooms.append(db_data->get_tag(r.toInt()));
            }
            QString room = rooms.join(",");
            int d = db_data->days.value(ldata.value("DAY").toInt());
            int h = db_data->hours.value(ldata.value("HOUR").toInt());
            Tile *t = new Tile(grid,
                QJsonObject {
                    {"TEXT", group},
                    {"TL", subject},
                    {"BR", room},
                    {"LENGTH", len},
                    {"DIV0", 0},
                    {"DIVS", 1},
                    {"NDIVS", 1},
                },
                lid
            );
            grid->place_tile(t, d, h);
        }
    }
}
