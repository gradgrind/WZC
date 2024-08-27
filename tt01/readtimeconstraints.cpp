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
            int gix = fet_info.groups.value(gid);
            auto gnode = &fet_info.nodes[gix];
            gnode->DATA["NOT_AVAILABLE"] = daylist;
        } else if (n.name == "ConstraintTeacherNotAvailableTimes") {
            auto m = readSimpleItems(n);
            auto tid = m.value("Teacher");
            QJsonArray daylist;
            for (const auto &vt : n.children) {
                auto nt = vt.value<XMLNode>();
                if (nt.name == "Not_Available_Time") {
                    auto ntdata = readSimpleItems(nt);
                    int d = fet_info.days.value(ntdata.value("Day"));
                    int h = fet_info.hours.value(ntdata.value("Hour"));
                    if (!d or !h) {
                        qFatal() << "ConstraintTeacherNotAvailableTimes"
                                 << "Teacher" << tid << "has invalid time:"
                                 << ntdata.value("Day") << "/"
                                 << ntdata.value("Hour");
                    }
                    daylist.append(QJsonArray{d, h});
                }
            }
            int tix = fet_info.teachers.value(tid);
            auto tnode = &fet_info.nodes[tix];
            tnode->DATA["NOT_AVAILABLE"] = daylist;
        }
// ConstraintActivityPreferredStartingTime
// ConstraintMinDaysBetweenActivities
// ConstraintMinDaysBetweenActivities
// ConstraintStudentsSetMaxGapsPerWeek
// ConstraintStudentsSetMinHoursDaily
// ConstraintStudentsSetMaxHoursDailyInInterval
// ConstraintTeacherMaxHoursDailyInInterval
// ConstraintTeachersMinHoursDaily
// ConstraintTeacherMaxDaysPerWeek
// ConstraintTeacherIntervalMaxDaysPerWeek
// ConstraintTeacherMaxGapsPerWeek
// ConstraintActivitiesPreferredTimeSlots
// ConstraintActivityPreferredStartingTimes
// ConstraintActivitiesSameStartingTime


    }

    /*
    for (auto i = fet_info.groups.cbegin(), end = fet_info.groups.cend();
        i != end; ++i) {
        auto gnode = fet_info.nodes.value(i.value());
        if (gnode.DATA.contains("NOT_AVAILABLE")) {
            qDebug() << "STUDENTSNOTAVAILABLE:" << i.key()
                     << gnode.DATA.value("NOT_AVAILABLE");
        }
    }
    for (auto i = fet_info.teachers.cbegin(), end = fet_info.teachers.cend();
         i != end; ++i) {
        auto tnode = fet_info.nodes.value(i.value());
        if (tnode.DATA.contains("NOT_AVAILABLE")) {
            qDebug() << "TEACHERNOTAVAILABLE:" << i.key()
            << tnode.DATA.value("NOT_AVAILABLE");
        }
    }
    */
}
