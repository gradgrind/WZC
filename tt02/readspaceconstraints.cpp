#include "readspaceconstraints.h"
#include <QJsonArray>

void readSpaceConstraints(FetInfo &fet_info, QList<QVariant> item_list)
{
    for (const auto &v : item_list) {
        auto n = v.value<XMLNode>();
        if (n.name == "ConstraintActivityPreferredRoom") {
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
// <ConstraintActivityPreferredRooms>
