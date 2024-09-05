#include "readtimeconstraints.h"
#include <QJsonArray>

// I use a weighting scale "-", "1", "2", ... "8", "9", "+".
// Perform a rough translation from the fet percentage weightings.
QString convert_weight(QString fet_weight)
{
    if (fet_weight == "100") return "+";
    if (fet_weight == "0") return "-";
    float w = fet_weight.toFloat();
    if (w < 93.5) {
        if (w < 83.6) {
            if (w < 60) return "1";
            if (w < 74) return "2";
            return "3";
        }
        if (w < 89.7) return "4";
        return "5";
    }
    if (w < 97.4) {
        if (w < 95.9) return "6";
        return "7";
    }
    if (w <= 98.5) return "8";
    return "9";
}

// Ignore "ConstraintBasicCompulsoryTime"

void readTimeConstraints(FetInfo &fet_info, QList<QVariant> item_list)
{
    for (const auto &v : item_list) {
        auto n = v.value<XMLNode>();
        //
        if (n.name == "ConstraintStudentsSetNotAvailableTimes") {
            auto m = readSimpleItems(n);
            auto gid = m.value("Students");
            //TODO: In fet this can be any group, not just a class.
            // Should I limit it to classes here, or add the info to the
            // groups? Currently I am adding it to the groups.
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
        //
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
        //
        } else if (n.name == "ConstraintActivityPreferredStartingTime") {
            auto m = readSimpleItems(n);
            auto aid = m.value("Activity_Id");
            auto day = m.value("Preferred_Day");
            auto hour = m.value("Preferred_Hour");
            bool fixed = m.value("Permanently_Locked") == "true";
            int aix = fet_info.activity_lesson.value(aid);
            auto anode = &fet_info.nodes[aix];
            anode->DATA["DAY"] = fet_info.days.value(day);
            anode->DATA["HOUR"] = fet_info.hours.value(hour);
            anode->DATA["FIXED"] = fixed;
        //
        } else if (n.name == "ConstraintMinDaysBetweenActivities") {
            auto m = readSimpleItems(n);
            QString w = convert_weight(m.value("Weight_Percentage"));
            QJsonArray lids;
            for (const auto &aid : m.values("Activity_Id")) {
                lids.append(fet_info.activity_lesson.value(aid));
            }
            // Add to LOCAL_CONSTRAINTS table.
            int hcid = fet_info.nodes.length();
            int days = m.value("MinDays").toInt();
            if (days == 1) {
                fet_info.nodes.append({
                    .Id = hcid,
                    .DB_TABLE = "LOCAL_CONSTRAINTS",
                    .DATA = {
                        {"TYPE", "ONE_DAY_BETWEEN"},
                        {"LESSONS", lids},
                        {"WEIGHT", w},
                    },
                });
            } else {
                fet_info.nodes.append({
                    .Id = hcid,
                    .DB_TABLE = "LOCAL_CONSTRAINTS",
                    .DATA = {
                        {"TYPE", "DAYS_BETWEEN"},
                        {"NDAYS", days},
                        {"LESSONS", lids},
                        {"WEIGHT", w},
                    },
                });
            }
        //
        } else if (n.name == "ConstraintActivityPreferredStartingTimes") {
            auto m = readSimpleItems(n);
            QString w = convert_weight(m.value("Weight_Percentage"));
            int lid = fet_info.activity_lesson.value(
                m.value("Activity_Id"));
            QJsonArray times;
            for (const auto &vt : n.children) {
                auto nt = vt.value<XMLNode>();
                if (nt.name == "Preferred_Starting_Time") {
                    auto ntdata = readSimpleItems(nt);
                    int d = fet_info.days.value(
                        ntdata.value("Preferred_Starting_Day"));
                    int h = fet_info.hours.value(
                        ntdata.value("Preferred_Starting_Hour"));
                    times.append(QJsonArray{d, h});
                }
            }
            // Add to LOCAL_CONSTRAINTS table.
            int hcid = fet_info.nodes.length();
            fet_info.nodes.append({
                .Id = hcid,
                .DB_TABLE = "LOCAL_CONSTRAINTS",
                .DATA = {
                    {"TYPE", "PREFERRED_STARTING_TIMES"},
                    {"LESSON", lid},
                    {"SLOTS", times},
                    {"WEIGHT", w},
                },
            });
        //
        } else if (n.name == "ConstraintActivitiesPreferredStartingTimes") {
            auto m = readSimpleItems(n);
            QString w = convert_weight(m.value("Weight_Percentage"));
            QString atag = m.value("Activity_Tag");
            int g = fet_info.groups.value(m.value("Students"));
            int s = fet_info.subjects.value(m.value("Subject"));
            int l = m.value("Duration").toInt();
            int t = fet_info.teachers.value(m.value("Teacher"));
            QJsonArray times;
            for (const auto &vt : n.children) {
                auto nt = vt.value<XMLNode>();
                if (nt.name == "Preferred_Starting_Time") {
                    auto ntdata = readSimpleItems(nt);
                    int d = fet_info.days.value(
                        ntdata.value("Preferred_Starting_Day"));
                    int h = fet_info.hours.value(
                        ntdata.value("Preferred_Starting_Hour"));
                    times.append(QJsonArray{d, h});
                }
            }
            // Add to LOCAL_CONSTRAINTS table.
            int hcid = fet_info.nodes.length();
            fet_info.nodes.append({
                .Id = hcid,
                .DB_TABLE = "LOCAL_CONSTRAINTS",
                .DATA = {
                    {"TYPE", "ACTIVITIES_PREFERRED_STARTING_TIMES"},
                    {"ACTIVITY_TAG", atag}, // TODO: these need reading in
                    {"TEACHER", t},
                    {"STUDENTS", g},
                    {"SUBJECT", s},
                    {"LENGTH", l},
                    {"SLOTS", times},
                    {"WEIGHT", w},
                },
            });
        //
        } else if (n.name == "ConstraintActivitiesPreferredTimeSlots") {
            auto m = readSimpleItems(n);
            QString w = convert_weight(m.value("Weight_Percentage"));
            QString atag = m.value("Activity_Tag");
            int g = fet_info.groups.value(m.value("Students"));
            int s = fet_info.subjects.value(m.value("Subject"));
            int l = m.value("Duration").toInt();
            int t = fet_info.teachers.value(m.value("Teacher"));
            QJsonArray times;
            for (const auto &vt : n.children) {
                auto nt = vt.value<XMLNode>();
                if (nt.name == "Preferred_Time_Slot") {
                    auto ntdata = readSimpleItems(nt);
                    int d = fet_info.days.value(
                        ntdata.value("Preferred_Day"));
                    int h = fet_info.hours.value(
                        ntdata.value("Preferred_Hour"));
                    times.append(QJsonArray{d, h});
                }
            }
            // Add to LOCAL_CONSTRAINTS table.
            int hcid = fet_info.nodes.length();
            fet_info.nodes.append({
                .Id = hcid,
                .DB_TABLE = "LOCAL_CONSTRAINTS",
                .DATA = {
                    {"TYPE", "ACTIVITIES_PREFERRED_TIME_SLOTS"},
                    {"ACTIVITY_TAG", atag}, // TODO: these need reading in
                    {"TEACHER", t},
                    {"STUDENTS", g},
                    {"SUBJECT", s},
                    {"LENGTH", l},
                    {"SLOTS", times},
                    {"WEIGHT", w},
                },
            });
        //
        } else if (n.name == "ConstraintActivitiesSameStartingTime") {
            auto m = readSimpleItems(n);
            QString w = convert_weight(m.value("Weight_Percentage"));
            QJsonArray lids;
            for (const auto &aid : m.values("Activity_Id")) {
                lids.append(fet_info.activity_lesson.value(aid));
            }
            // Add to LOCAL_CONSTRAINTS table.
            int hcid = fet_info.nodes.length();
            fet_info.nodes.append({
                .Id = hcid,
                .DB_TABLE = "LOCAL_CONSTRAINTS",
                .DATA = {
                    {"TYPE", "SAME_STARTING_TIME"},
                    {"LESSONS", lids},
                    {"WEIGHT", w},
                },
            });
        }

// ConstraintStudentsSetMaxGapsPerWeek
// ConstraintStudentsSetMinHoursDaily
// ConstraintStudentsSetMaxHoursDailyInInterval
// ConstraintTeacherMaxHoursDailyInInterval
// ConstraintTeachersMinHoursDaily
// ConstraintTeacherMaxDaysPerWeek
// ConstraintTeacherIntervalMaxDaysPerWeek
// ConstraintTeacherMaxGapsPerWeek

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
