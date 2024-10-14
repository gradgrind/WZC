#include "softactivitytimes.h"

SoftActivityTimes::SoftActivityTimes(
    BasicConstraints *constraint_data,
    int weight,
    // For each day a list of allowed times
    slot_constraint &ttslots,
    bool allslots) : all_slots{allslots}
{
    penalty = weight;
    week_slots.resize(
        constraint_data->ndays,
        std::vector<bool>(constraint_data->nhours, false)
        );
    for (int d = 0; d < constraint_data->ndays; ++d ) {
        for (int h : ttslots.at(d)) {
            week_slots.at(d).at(h) = true;
        }
    }
    //TODO--
    //    qDebug() << "SoftActivityTimes" << penalty << all_slots << lesson_indexes;
}

void SoftActivityTimes::add_lesson_id(
    BasicConstraints *constraint_data,
    int lesson_id)
{
    lesson_indexes.push_back(constraint_data->lid2lix.at(lesson_id));
}

//TODO
int SoftActivityTimes::evaluate(BasicConstraints *constraint_data) { return 0; }


void multi_slot_constraints(
    BasicConstraints * bcp,
    std::vector<ActivitySelectionSlots> &alist,
    bool allslots // false for starting times, true for slots
    ) {
    for (auto &a : alist) {
        SoftActivityTimes *sat = nullptr; // only needed for soft constraints
        bool hard = a.isHard();
        if (!hard) {
            sat = new SoftActivityTimes(
                bcp,
                a.weight,
                a.ttslots,
                allslots
            );
            bcp->general_constraints.push_back(sat);
        }
        for (int lix = 1; lix < bcp->lessons.size(); ++lix) {
            auto &ld = bcp->lessons[lix];
            if ((a.tag.isEmpty() || ld.tags.contains(a.tag))
                && (!a.tid ||
                    (std::find(
                        ld.teachers.begin(),
                        ld.teachers.end(),
                        bcp->t2i.value(a.tid)) != ld.teachers.end()
                    )
                )
                && (!a.gid ||
                    // Test all atomic groups included in a.gid
                    [&]() -> bool {
                        for (const auto &sg : bcp->g2sg.value(a.gid)) {
                           int agi = bcp->sg2i.value(sg);
                           if (std::find(
                                ld.atomic_groups.begin(),
                                ld.atomic_groups.end(),
                                agi) != ld.atomic_groups.end()
                            ) return true;
                        }
                        return false;
                    }()
                )
                && (!a.sid || a.sid == ld.subject)
                && (!a.l || a.l == ld.length)) {
                // The lesson matches
                if (hard) {
                    // If they are starting times, simply merge them into the
                    // existing array.
                    // If they are available slots, the possible starting
                    // times need to be built, based on the lesson length.
                    // So start with that ...
                    if (allslots && ld.length > 1) {
                        slot_constraint ttslots(bcp->ndays);
                        int d = 0;
                        for (int d = 0; d < bcp->ndays; ++d) {
                            const auto &dvec = a.ttslots.at(d);
                            int start = -1;
                            int l = 0;
                            for (int h : dvec) {
                                if (start < 0) {
                                    start = h;
                                    l = 1;
                                } else if (start + l == h) {
                                    // contiguous
                                    if (++l == ld.length) {
                                        ttslots[d].push_back(start);
                                        ++start;
                                        --l;
                                    }
                                } else {
                                    // not contiguous, restart
                                    start = h;
                                    l = 1;
                                }
                            }
                        }
                        a.ttslots = ttslots;
                    }
                    // Merge in new start-slots
                    bcp->merge_slot_constraints(ld, a.ttslots);
                } else {
                    // Soft constraint
                    sat->add_lesson_id(bcp, ld.lesson_id);
                }
            }
        }
    }
}
