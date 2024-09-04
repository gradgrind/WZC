#include "readspaceconstraints.h"
#include <QJsonArray>

void readSpaceConstraints(FetInfo &fet_info, QList<QVariant> item_list)
{
    for (const auto &v : item_list) {
        auto n = v.value<XMLNode>();
        // I don't completely understand the use of the Weight_Percentage
        // value here ... I will assume always 100%.
        if (n.name == "ConstraintActivityPreferredRooms") {
            auto m = readSimpleItems(n);
            auto aid = m.value("Activity_Id");
            // Actually, roomspecs should surely be on courses, not lessons.
            // So some entries will be redundant. I'm not sure whether fet
            // can write different roomspecs for the various lessons of a
            // course, but they are written to the individual activities.
            // Assume all roomspecs of a course are identical and don't
            // overwrite an existing roomspec value for a course with
            // multiple lessons.
            int aix = fet_info.activity_lesson.value(aid);
            int cid = fet_info.nodes[aix].DATA.value("COURSE").toInt();
            auto cnode = &fet_info.nodes[cid];
            // Don't overwrite an existing roomspec value
            if (cnode->DATA.contains("ROOMSPEC")) continue;
            QJsonArray room_list;
            auto fet_room_list = m.values("Preferred_Room");
            for (const auto &r : fet_room_list) {
                int rr = fet_info.rooms.value(r);
                // If this is a "virtual" room, it should be the only room
                // in this list (to avoid overcomplexity ...)
                auto node = fet_info.nodes.value(rr).DATA;
                if (node.contains("ROOMS_NEEDED")
                        and (fet_room_list.length() != 1)) {
                    qFatal() << "Room constraint, virtual room is not"
                             << "alone on activity" << aid;
                }
                room_list.append(rr);
            }
            cnode->DATA["ROOMSPEC"] = room_list;
        } else if (n.name == "ConstraintActivityPreferredRoom") {
            // If the acceptable room is initially set using this
            // constraint, there may be two entries. The later one
            // will be the one containing "Real_Room" entries, if the
            // Room is virtual. The later value overrides the earlier one.
            // Nevertheless, it seems to be better to use
            // "ConstraintActivityPreferredRooms" to specify acceptable
            // rooms, even if there is only one.
            auto m = readSimpleItems(n);
            auto aid = m.value("Activity_Id");
            QJsonArray room_list;
            auto real_rooms = m.values("Real_Room");
            if (real_rooms.isEmpty()) {
                room_list.append(fet_info.rooms.value(m.value("Room")));
            } else {
                for (const auto &r : real_rooms) {
                    room_list.append(fet_info.rooms.value(r));
                }
            }
            int aix = fet_info.activity_lesson.value(aid);
            auto anode = &fet_info.nodes[aix];
            anode->DATA["ROOMS"] = room_list;
            //qDebug() << "ROOM" << aid << fet_info.nodes[aix].DATA;
        }
    }
}

// <Space_Constraints_List>
// <ConstraintBasicCompulsorySpace>
