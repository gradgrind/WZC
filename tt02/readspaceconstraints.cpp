#include "readspaceconstraints.h"
#include <QJsonArray>

void readSpaceConstraints(FetInfo &fet_info, QList<QVariant> item_list)
{
    // FET's handling of room constraints is a bit complicated.
    // I don't completely understand the use of the Weight_Percentage
    // value, but my model only caters for 100%. Only constraints with this
    // weighting will be repected. I will also only deal with
    // fairly straightforward situations. If conflicting constraints are
    // possible, these will be ignored, information from earlier constraints
    // can be overwritten by later ones.

    // In a FET file, the room specifications are associated with Activities,
    // which correspond to lessons. I assume that room specifications are
    // associated rather with courses. So some entries in the FET file will
    // be redundant. I'm not sure whether FET can actually write different
    // room specifications for the various lessons of a course, but I will
    // ignore the possibility and assume all the lessons of a course have
    // the same room specification.

    for (const auto &v : item_list) {
        auto n = v.value<XMLNode>();
        if (n.name == "ConstraintActivityPreferredRooms") {
            auto m = readSimpleItems(n);
            if (m.value("Weight_Percentage") != "100") continue;
            auto aid = m.value("Activity_Id");
            int aix = fet_info.activity_lesson.value(aid);
            int cid = fet_info.nodes[aix].value("COURSE").toInt();
            auto &cnode = fet_info.nodes[cid];
            // Don't overwrite an existing room specification
            if (cnode.contains("FIXED_ROOMS")
                or cnode.contains("ROOM_CHOICE")) continue;
            auto fet_room_list = m.values("Preferred_Room");
            if (fet_room_list.length() == 1) {
                auto r = fet_room_list[0];
                int rr = fet_info.rooms.value(r);
                auto node = fet_info.nodes.value(rr);
                if (node.contains("FIXED_ROOMS")) {
                    // Virtual room
                    cnode["FIXED_ROOMS"] = node["FIXED_ROOMS"];
                    if (node.contains("ROOM_CHOICE")) {
                        cnode["ROOM_CHOICE"] = node["ROOM_CHOICE"];
                    }
                } else {
                    cnode["FIXED_ROOMS"] = QJsonArray{rr};
                }
            } else {
                // Multiple rooms (choice)
                QJsonArray room_list;
                for (const auto &r : fet_room_list) {
                    int rr = fet_info.rooms.value(r);
                    // Virtual rooms are not permitted here (to avoid
                    //  overcomplexity ...)
                    auto node = fet_info.nodes.value(rr);
                    if (node.contains("FIXED_ROOMS"))
                        qFatal() << "Room constraint: virtual room is not"
                                 << "alone on activity" << aid;
                    else {
                        room_list.append(rr);
                    }
                }
                cnode["ROOM_CHOICE"] = room_list;
            }
        } else if (n.name == "ConstraintActivityPreferredRoom") {
            // If the acceptable room is initially set using this
            // constraint, there may be two entries. The later one
            // will be the one containing "Real_Room" entries, if the
            // Room is virtual. To keep it simple it seems
            // better to use "ConstraintActivityPreferredRooms"
            // to specify acceptable rooms, even if there is only one.
            auto m = readSimpleItems(n);
            if (m.value("Weight_Percentage") != "100") continue;
            auto aid = m.value("Activity_Id");
            int aix = fet_info.activity_lesson.value(aid);
            int cid = fet_info.nodes[aix].value("COURSE").toInt();
            auto &cnode = fet_info.nodes[cid];
            auto real_rooms = m.values("Real_Room");
            if (real_rooms.empty()) {
                // If this is a virtual room, this is the specification.
                // For a real room it can be both specification and actual
                // occupation.
                auto r = m.value("Room");
                int rr = fet_info.rooms.value(r);
                auto node = fet_info.nodes.value(rr);
                if (node.contains("FIXED_ROOMS")) {
                    // Virtual room
                    cnode["FIXED_ROOMS"] = node["FIXED_ROOMS"];
                    if (node.contains("ROOM_CHOICE")) {
                        cnode["ROOM_CHOICE"] = node["ROOM_CHOICE"];
                    }
                } else {
                    // Real room.
                    // If there is no existing choice specification, take
                    // this as the fixed room.
                    if (!cnode.contains("ROOM_CHOICE")) {
                        cnode["FIXED_ROOMS"] = QJsonArray{rr};
                    }
                    auto &anode = fet_info.nodes[aix];
                    anode["FLEXIBLE_ROOM"] = rr;
                }
            } else {
                // This lists the actually used rooms of a virtual room.
                // If there is a flexible room it must be found and set in
                // the lesson. That requires an existing room specification.
                auto needed = cnode.value("FIXED_ROOMS").toArray();
                QList<int> spare;
                for (const auto &r : real_rooms) {
                    int rr = fet_info.rooms.value(r);
                    if (!needed.contains(rr)) spare.append(rr);
                }
                if (!spare.empty()) {
                    if (spare.size() > 1) qWarning() << "Bad room specification"
                                   << "for Activity" << aix;
                    auto choice = cnode.value("ROOM_CHOICE").toArray();
                    for (int rr : spare) {
                        if (choice.contains(rr)) {
                            auto &anode = fet_info.nodes[aix];
                            anode["FLEXIBLE_ROOM"] = spare[0];
                            break;
                        }
                    }
                }
            }
            //qDebug() << "ROOM" << aid << fet_info.nodes[aix];
        }
    }
}

// <Space_Constraints_List>
// <ConstraintBasicCompulsorySpace>
