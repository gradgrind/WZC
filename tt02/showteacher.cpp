#include "showteacher.h"
#include <QJsonArray>

ShowTeacher::ShowTeacher(TT_Grid *grid, TimetableData *tt_data, int teacher_id)
{
    DBData *db_data = tt_data->db_data;
    for (int course_id : tt_data->teacher_courses[teacher_id]) {
        auto course = db_data->Nodes.value(course_id);
        QStringList groups;
        auto glist = course.value("GROUPS").toArray();
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
        QStringList rooms;
        const auto rlist = course.value("FIXED_ROOMS").toArray();
        for (const auto & r : rlist) {
            rooms.append(db_data->get_tag(r.toInt()));
        }
        for (int lid : db_data->course_lessons.value(course_id)) {
            auto ldata = db_data->Nodes.value(lid);
            int len = ldata.value("LENGTH").toInt();
            // Add possible chosen room
            QStringList roomlist(rooms);
            auto fr{ldata.value("FLEXIBLE_ROOM")};
            if (!fr.isUndefined()) roomlist.append(db_data->get_tag(fr.toInt()));
            int d0 = ldata.value("DAY").toInt();
            if (d0 == 0) {
//TODO: Collect unplaced lessons

            } else {
                int d = db_data->days.value(d0);
                int h = db_data->hours.value(ldata.value("HOUR").toInt());
                Tile *t = new Tile(grid,
                    QJsonObject {
                        {"TEXT", group},
                        {"TL", subject},
                        {"BR", roomlist.join(",")},
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
}
