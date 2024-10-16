#include <qjsonarray.h>
#include "localconstraints.h"
#include "differentdays.h"
#include "samestartingtime.h"
#include "softactivitytimes.h"

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
            slot_constraint days(basic_constraints->ndays);
            const auto &ttslots = node.value("SLOTS").toArray();
            for (auto ttslot : ttslots) {
                auto dhslot = ttslot.toArray();
                int d = db_data->days.value(dhslot.at(0).toInt());
                int h = db_data->hours.value(dhslot.at(1).toInt());
                days[d].push_back(h);
            }
            if (ntype == "PREFERRED_STARTING_TIMES") {
                int lid = node.value("LESSON").toInt();
                if (is_hard(w)) {
                    basic_constraints->set_start_cells_id(lid, days);
                } else  {
                    auto sat = new SoftActivityTimes(
                        basic_constraints, w, days, false);
                    sat->add_lesson_id(basic_constraints, lid);
                    basic_constraints->general_constraints.push_back(sat);
                }
            } else {
                if (ntype == "ACTIVITIES_PREFERRED_STARTING_TIMES") {
                    constraints.activities_starting_times.push_back({
                        .weight = w,
                        .tag = node.value("ACTIVITY_TAG").toString(),
                        .tid = node.value("TEACHER").toInt(),
                        .gid = node.value("GROUP").toInt(),
                        .sid = node.value("SUBJECT").toInt(),
                        .l = node.value("LENGTH").toInt(),
                        .ttslots = days,
                    });
                } else if (ntype == "ACTIVITIES_PREFERRED_TIME_SLOTS") {
                    constraints.activities_slots.push_back({
                        .weight = w,
                        .tag = node.value("ACTIVITY_TAG").toString(),
                        .tid = node.value("TEACHER").toInt(),
                        .gid = node.value("GROUP").toInt(),
                        .sid = node.value("SUBJECT").toInt(),
                        .l = node.value("LENGTH").toInt(),
                        .ttslots = days,
                    });
                } else {
                    qFatal() << "Unexpected constraint:" << ntype;
                }
            }
        } else if (ntype == "DAYS_BETWEEN") {
            if (is_hard(w) && node.value("NDAYS") == 1) {
                const auto llist = node.value("LESSONS").toArray();
                std::vector<int> lids(llist.size());
                for (int i = 0; i < llist.size(); ++i) {
                    lids[i] = llist[i].toInt();
                }
                basic_constraints->set_different_days(lids);
            } else {
                DifferentDays * dd = new DifferentDays(basic_constraints, node);
                basic_constraints->general_constraints.push_back(dd);
            }
        } else if (ntype == "SAME_STARTING_TIME") {
            auto llist = node.value("LESSONS").toArray();
            //qDebug() << "$$$ SAME_STARTING_TIME:" << llist;
            int n = llist.size();
            std::vector<int> lesson_indexes(n);
            for (int i = 0; i < n; ++i) {
                lesson_indexes[i] = basic_constraints->lid2lix[llist[i].toInt()];
            }
            int weight = node.value("WEIGHT").toInt();
            if (is_hard(weight)) {
                constraints.parallel_lessons.push_back(lesson_indexes);
            } else {
                basic_constraints->general_constraints.push_back(
                    new SameStartingTime(lesson_indexes, weight));
            }
        } else {
            qFatal() << "Unexpected constraint:" << ntype;
        }
    }
    return constraints;
}

void localConstraints(BasicConstraints *basic_constraints)
{
    // Build the BasicConstraints::lessons list, placing the fixed lessons
    // and returning a list of the unfixed ones.
    basic_constraints->initial_place_lessons();
    // Collect the hard local constraints which specify possible
    // starting times for individual lessons or lessons fulfilling
    // certain conditions. Also the lesson lengths are taken into account.
    time_constraints tconstraints = activity_slot_constraints(basic_constraints);
    // Set up the slot_constraints field (for unfixed lessons). This field
    // specifies which slots can potentially be used for the lesson – assuming
    // no basic clashes.
    // Parallel lessons need to share slot_constraints.
    basic_constraints->setup_parallels(tconstraints.parallel_lessons);
    // Include further restrictions on starting times, from constraints
    // concerning multiple activities
    multi_slot_constraints(
        basic_constraints,
        tconstraints.activities_starting_times,
        false
    );
    multi_slot_constraints(
        basic_constraints,
        tconstraints.activities_slots,
        true
    );
    // Place the (unfixed) lessons which have their starting times specified.
    basic_constraints->initial_place_lessons2(tconstraints);
}

