#include "showroom.h"
#include <QJsonArray>

ShowRoom::ShowRoom(TT_Grid *grid, DBData *db_data, int room_id)
{
    QJsonValue r(room_id);
    // I need to go through all the lessons
    for (int lid : db_data->Tables.value("LESSONS")) {
        auto node = db_data->Nodes.value(lid);
        if (!node.value("ROOMS").toArray().contains(r)) continue;

        int course_id = node.value("COURSE").toInt();
        auto course = db_data->Nodes.value(course_id);
        QStringList teachers;
        auto tlist = course.value("TEACHERS").toArray();
        for (const auto & t : tlist) {
            teachers.append(db_data->get_tag(t.toInt()));
        }
        QString teacher = teachers.join(",");
        QStringList groups;
        auto glist = course.value("STUDENTS").toArray();
        for (const auto & g : glist) {
            // Combine class and group
            auto node = db_data->Nodes.value(g.toInt());
            auto gtag = node.value("ID").toString();
            auto ctag = db_data->get_tag(node.value("CLASS").toInt());
            if (gtag.isEmpty()) {
                groups.append(ctag);
            } else {
                groups.append(ctag + "." + gtag);
            }
        }
        QString group = groups.join(",");
        QString subject = db_data->get_tag(course.value("SUBJECT").toInt());
        int d = db_data->days.value(node.value("DAY").toInt());
        int h = db_data->hours.value(node.value("HOUR").toInt());
        Tile *t = new Tile(grid,
            QJsonObject {
                {"TEXT", group},
                {"TL", subject},
                {"BR", teacher},
                {"LENGTH", node.value("LENGTH").toInt()},
                {"DIV0", 0},
                {"DIVS", 1},
                {"NDIVS", 1},
            },
            lid
        );
        grid->place_tile(t, d, h);
    }
}
