#include <qjsonarray.h>
#include "localconstraints.h"
#include "differentdays.h"

// Collect Activity slot constraints
time_constraints activity_slot_constraints(BasicConstraints *basic_constraints)
{
    time_constraints constraints;
    auto db_data = basic_constraints->db_data;
    for (int xid : db_data->Tables.value("LOCAL_CONSTRAINTS")) {
        auto node = db_data->Nodes.value(xid);
        int w = node.value("WEIGHT").toInt();
        auto ntype = node.value("CTYPE");
        if (node.contains("SLOTS")) {
            //NOTE: I assume the times are sorted in the SLOTS list.
            std::vector<std::vector<int>> days(basic_constraints->ndays);
            const auto &ttslots = node.value("SLOTS").toArray();
            for (auto ttslot : ttslots) {
                auto dhslot = ttslot.toArray();
                int d = db_data->days.value(dhslot.at(0).toInt());
                int h = db_data->hours.value(dhslot.at(1).toInt());
                days[d].push_back(h);
            }
            if (ntype == "PREFERRED_STARTING_TIMES") {
                int lid = node.value("LESSON").toInt();
                constraints.lesson_starting_times[lid] = {
                    .weight = w, .ttslots = days};
            } else {
                if (ntype == "ACTIVITIES_PREFERRED_STARTING_TIMES") {
                    constraints.activities_starting_times.push_back({
                        .weight = w,
                        .tag = node.value("ACTIVITY_TAG").toString(),
                        .tid = node.value("TEACHER").toInt(),
                        .gid = node.value("GROUPS").toInt(),
                        .sid = node.value("SUBJECT").toInt(),
                        .l = node.value("LENGTH").toInt(),
                        .ttslots = days,
                    });
                } else if (ntype == "ACTIVITIES_PREFERRED_TIME_SLOTS") {
                    constraints.activities_slots.push_back({
                        .weight = w,
                        .tag = node.value("ACTIVITY_TAG").toString(),
                        .tid = node.value("TEACHER").toInt(),
                        .gid = node.value("GROUPS").toInt(),
                        .sid = node.value("SUBJECT").toInt(),
                        .l = node.value("LENGTH").toInt(),
                        .ttslots = days,
                    });
                } else {
                    qFatal() << "Unexpected constraint:" << ntype;
                }
            }
        } else if (ntype == "DAYS_BETWEEN") {
            DifferentDays * dd = new DifferentDays(basic_constraints, node);
            if (dd->isHard()) {
                basic_constraints->local_hard_constraints.push_back(dd);
                for (int lix : dd->lesson_indexes) {
                    basic_constraints->lessons.at(lix)
                        .day_constraints.push_back(dd);
                }
            } else {
                basic_constraints->general_constraints.push_back(dd);
            }
        } else if (ntype == "SAME_STARTING_TIME") {
            SameStartingTime * sst = new SameStartingTime(
                basic_constraints, node);
            if (sst->isHard()) {
                basic_constraints->local_hard_constraints.push_back(sst);
                for (int lid : sst->lesson_indexes) {
                    auto l = &basic_constraints->lessons[lid];
                    if (l->parallel) {
                        qDebug() << "Lesson" << l->lesson_id
                                 << "has multiple 'SameStartingTime' constraints";
                        continue;
                    }
                    l->parallel = sst;
                }
            } else {
                basic_constraints->general_constraints.push_back(sst);
            }
        }
    }
    return constraints;
}

void localConstraints(BasicConstraints *basic_constraints)
{
    // Build the BasicConstraints::lessons list, placing the fixed lessons
    // and returning a list of the unfixed ones.
    auto unplaced = basic_constraints->initial_place_lessons();
    // Collect the hard local constraints which specify possible
    // starting times for individual lessons or lessons fulfilling
    // certain conditions. Also the lesson lengths are taken into account.
    time_constraints tconstraints = activity_slot_constraints(basic_constraints);
    // Place the (unfixed) lessons which have their starting times specified.
    // Also set up the start_cells field (for unfixed lessons). This field
    // specifies which slots can potentially be used for the lesson – assuming
    // no basic clashes.
    basic_constraints->initial_place_lessons2(unplaced, tconstraints);
}

