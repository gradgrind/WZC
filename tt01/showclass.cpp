#include "showclass.h"
#include <QJsonArray>

ShowClass::ShowClass(DBData & db_data, int class_id)
{
    for (int course_id : db_data.class_courses[class_id]) {
        auto course = db_data.Nodes.value(course_id).DATA;
        QStringList teachers;
        auto tlist = course.value("TEACHERS").toArray();
        for (const auto & t : tlist) {
            teachers.append(db_data.get_tag(t.toInt()));
        }
        QString subject = db_data.get_tag(course.value("SUBJECT").toInt());
        // The rooms need to be done on a lesson basis!
        auto tile_info = db_data.course_tileinfo[course_id];
        auto tiles = tile_info.value(class_id);
    }
}
