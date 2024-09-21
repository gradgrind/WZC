#include <qjsonarray.h>
#include "localconstraints.h"
#include "samestartingtime.h"

// Collect Activity slot constraints
time_constraints activity_slot_constraints(BasicConstraints *basic_constraints)
{
    time_constraints constraints;
    auto db_data = basic_constraints->db_data;
    for (int xid : db_data->Tables.value("LOCAL_CONSTRAINTS")) {
        auto node = db_data->Nodes.value(xid).DATA;
        if (node.value("WEIGHT") != "+") continue; // only hard constraints

        auto ntype = node.value("TYPE");
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
                constraints.lesson_starting_times[lid] = days;
            } else {
                std::vector<ActivitySelectionSlots> *alist;
                if (ntype == "ACTIVITIES_PREFERRED_STARTING_TIMES") {
                    alist = &constraints.activities_starting_times;
                } else if (ntype == "ACTIVITIES_PREFERRED_TIME_SLOTS") {
                    alist = &constraints.activities_slots;
                } else {
                    qFatal() << "Unexpected constraint:" << ntype;
                }
                alist->push_back({
                    .tag = node.value("ACTIVITY_TAG").toString(),
                    .tid = node.value("TEACHER").toInt(),
                    .gid = node.value("STUDENTS").toInt(),
                    .sid = node.value("SUBJECT").toInt(),
                    .l = node.value("LENGTH").toInt(),
                    .ttslots = days,
                });
            }
        } else if (ntype == "DAYS_BETWEEN") {
            //                {"NDAYS", days},
            //                    {"LESSONS", lids},
            //                    {"WEIGHT", w},


                } else if (ntype == "SAME_STARTING_TIME") {
                    SameStartingTime sst(node);

            //            {"LESSONS", lids},
            //                {"WEIGHT", w},
            }
    }
    return constraints;
}

void localConstraints(BasicConstraints *basic_constraints)
{
    // Collect the hard local constraints which specify possible
    // starting times for individual lessons or lessons fulfilling
    // certain conditions. Also the lesson lengths are taken into account.
    time_constraints tconstraints = activity_slot_constraints(basic_constraints);
    // Place the lessons which have their starting times specified.
    // Also set up the start_cells field for non-fixed lessons.
    basic_constraints->initial_place_lessons(tconstraints);
}

