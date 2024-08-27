#include "readtimeconstraints.h"
#include <QJsonArray>

// Ignore "ConstraintBasicCompulsoryTime"

void readTimeConstraints(FetInfo &fet_info, QList<QVariant> item_list)
{
    for (const auto &v : item_list) {
        auto n = v.value<XMLNode>();
        if (n.name == "ConstraintStudentsSetNotAvailableTimes") {
            auto m = readSimpleItems(n);
            auto gid = m.value("Students");
            //TODO: In fet this can be any group, not just a class.
            // Should I limit it to classes here, or add the info to the groups?
            // Currently I am adding it to the groups.
            QJsonArray daylist;
            for (const auto &vt : n.children) {
                auto nt = vt.value<XMLNode>();
                if (nt.name == "Not_Available_Time") {
                    auto ntdata = readSimpleItems(nt);
                    int d = fet_info.days.value(ntdata.value("Day"));
                    int h = fet_info.hours.value(ntdata.value("Hour"));
                    if (!d or !h) {
                        qFatal() << "ConstraintStudentsSetNotAvailableTimes"
                                 << "Group" << gid << "has invalid time:"
                                 << ntdata.value("Day") << "/"
                                 << ntdata.value("Hour");
                    }
                    daylist.append(QJsonArray{d, h});
                }

            }
            //qDebug() << "STUDENTSNOTAVAILABLE:" << gid << daylist;
            int gix = fet_info.groups.value(gid);
            auto gnode = &fet_info.nodes[gix];
            gnode->DATA["NOT_AVAILABLE"] = daylist;
        }
    }

    for (auto i = fet_info.groups.cbegin(), end = fet_info.groups.cend();
        i != end; ++i) {
        auto gnode = fet_info.nodes.value(i.value());
        if (gnode.DATA.contains("NOT_AVAILABLE")) {
            qDebug() << "STUDENTSNOTAVAILABLE:" << i.key()
                     << gnode.DATA.value("NOT_AVAILABLE");
        }
    }
}
