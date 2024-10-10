#include "basicconstraints.h"
#include <QJsonArray>

// Apply a filter to the set of permitted slots belonging to a lesson.
// The filter array is of the same form as the original structure, a
// slot_constraint item – a vector of integer-vectors containing the
// permitted hours (periods) for each day.
// The original is modified in-place if its slot_constraints index is
// non-zero, otherwise a new entry is made for the lesson with the values
// as in the modifier.
// Return the slot_constraints index.
void BasicConstraints::merge_slot_constraints(
    LessonData &ldata, const slot_constraint &newslots)
{
    int ci = ldata.slot_constraint_index;
    if (ci == 0) {
        // Need a new entry in start_cells_list
        ci = start_cells_list.size();
        start_cells_list.push_back(newslots);
        ldata.slot_constraint_index = ci;
    } else {
        slot_constraint &myslots = start_cells_list.at(ci);
        for (int d = 0; d < ndays; ++d) {
            auto &dayslots = myslots.at(d);
            if (dayslots.empty()) continue;
            int x = 0; // source read index
            int i = 0; // source write index
            int h0 = dayslots[0];
            for (int h : newslots[d]) {
                if (h < 0) continue;    // allow "dummy" entries
                if (h >= h0) {
                    if (h == h0) {
                        if (i != x) dayslots[i] = h;
                        ++i;
                    }
                    ++x;
                    if (x < dayslots.size()) {
                        h0 = dayslots[x];
                    } else {
                        // No more source hours
                        break;
                    }
                }
                // else (h < h0) -> next h
            }
            dayslots.resize(i);
        }
    }
}

int BasicConstraints::set_start_cells(
    LessonData &ldata, slot_constraint &week_slots)
{
    if (ldata.slot_constraint_index != 0)
        qFatal() << "Multiple starting times constraints for"
                 << "lesson id" << ldata.lesson_id;
    int ci = start_cells_list.size();
    start_cells_list.push_back(week_slots);
    ldata.slot_constraint_index = ci;
    return ci;
}

void BasicConstraints::set_start_cells_id(
    int lid, slot_constraint &week_slots)
{
    int lix = lid2lix.at(lid);
    set_start_cells(lessons.at(lix), week_slots);
}

BasicConstraints::BasicConstraints(DBData *dbdata) : db_data{dbdata}
{
    // Each "resource" – (atomic) student group, teacher and room –
    // has a vector of slots for one week, organized as days * hours.
    // When the resource is "busy" in a time slot, the slot will contain
    // the index of the lesson-node.

    // Collect the atomic groups
    for (int gid : dbdata->Tables.value("GROUPS")) {
        auto node = dbdata->Nodes.value(gid);
        auto sglist = node.value("SUBGROUPS").toArray();
        bool allsg = node.value("TAG").toString().isEmpty();
        for (auto sg : sglist) {
            auto sgstr = sg.toString();
            g2sg[gid].append(sgstr);
            if (allsg) {
                // A whole-class group
                sg2i[sgstr] = i_sg.length();
                i_sg.append(sgstr);
            }
        }
    }
    //qDebug() << "\n!!! g2sg:" << g2sg;
    //qDebug() << "\n!!! sg2i:" << sg2i;

    // Make a weekly array for each atomic subgroup
    ndays = dbdata->days.size();
    nhours = dbdata->hours.size();
    sg_weeks = std::vector<slot_constraint>(
        i_sg.length(), slot_constraint (
            ndays , std::vector<int> (nhours)));
    /*
    qDebug() << "ndays:" << ndays << "|| nhours:" << nhours;
    qDebug() << "sg_weeks:"
             << sg_weeks.size() << "*"
             << sg_weeks[0].size() << "*"
             << sg_weeks[0][0].size();
    */

    // Make a weekly array for each teacher
    for (int tid : dbdata->Tables.value("TEACHERS")) {
        t2i[tid] = i_t.length();
        i_t.append(tid);
    }
    t_weeks = std::vector<slot_constraint>(
        i_t.length(), slot_constraint (
            ndays , std::vector<int> (nhours)));
    /*
    qDebug() << "t_weeks:"
             << t_weeks.size() << "*"
             << t_weeks[0].size() << "*"
             << t_weeks[0][0].size();
    */

    // Make a weekly array for each real room
    for (int rid : dbdata->Tables.value("ROOMS")) {
        auto node = dbdata->Nodes.value(rid);
        if (node.contains("ROOM_CHOICE")) continue;
        // A real room
        r2i[rid] = i_r.length();
        i_r.append(rid);
    }
    r_weeks = std::vector<slot_constraint>(
        i_r.length(), slot_constraint (
            ndays , std::vector<int> (nhours)));

    // First enter the NOT_AVAILABLE constraints into the week-vectors
    // for teachers, student groups and rooms.
    slot_blockers();

    // Prepare the start_cells_list
    start_cells_list.reserve(dbdata->Tables.value("LESSONS").size() + 1);
    // Construct a non-restrictive start-cells array for the null lesson.
    start_cells_list.push_back(slot_constraint(ndays, std::vector<int>(nhours)));
    auto & sc0 = start_cells_list[0];
    for (int d = 0; d < ndays; ++d) {
        for (int h = 0; h < nhours; ++h) {
            sc0[d][h] = h;
        }
    }
    //qDebug() << "\n???????? start_cells_list[0]";
    //for (const auto &dlist: start_cells_list[0]) qDebug() << dlist;
}

void BasicConstraints::slot_blockers()
{
    // Block slots where teachers are "not available"
    for (int tid : db_data->Tables.value("TEACHERS")) {
        auto node = db_data->Nodes.value(tid);
        auto blist = node.value("NOT_AVAILABLE").toArray();
        for (auto b : blist) {
            auto bpair = b.toArray();
            int d = db_data->days.value(bpair.at(0).toInt());
            int h = db_data->hours.value(bpair.at(1).toInt());
            //qDebug() << db_data->get_tag(tid) << d << h;
            t_weeks.at(t2i.value(tid)).at(d).at(h) = -1;
        }
    }
    // Block slots where student groups are "not available"
    for (int gid : db_data->Tables.value("GROUPS")) {
        auto node = db_data->Nodes.value(gid);
        auto blist = node.value("NOT_AVAILABLE").toArray();
        if (blist.isEmpty()) continue;
        auto sglist = node.value("SUBGROUPS").toArray();
        for (auto b : blist) {
            auto bpair = b.toArray();
            int d = db_data->days.value(bpair.at(0).toInt());
            int h = db_data->hours.value(bpair.at(1).toInt());
            for (auto sg : sglist) {
                sg_weeks.at(sg2i.value(sg.toString())).at(d).at(h) = -1;
            }
        }
    }
    // Block slots where rooms are "not available"
    for (int rid : db_data->Tables.value("ROOMS")) {
        auto node = db_data->Nodes.value(rid);
        // Don't bother with virtual rooms:
        if (node.contains("FIXED_ROOMS")) continue;
        auto blist = node.value("NOT_AVAILABLE").toArray();
        for (auto b : blist) {
            auto bpair = b.toArray();
            int d = db_data->days.value(bpair.at(0).toInt());
            int h = db_data->hours.value(bpair.at(1).toInt());
            //qDebug() << db_data->get_tag(tid) << d << h;
            t_weeks.at(r2i.value(rid)).at(d).at(h) = -1;
        }
    }
}

void BasicConstraints::multi_slot_constraints(
    std::vector<ActivitySelectionSlots> &alist,
    bool allslots // false for starting times, true for slots
) {
    for (auto &a : alist) {
        SoftActivityTimes *sat = nullptr; // only needed for soft constraints
        bool hard = a.isHard();
        if (!hard) {
            sat = new SoftActivityTimes(
                this,
                a.weight,
                a.ttslots,
                allslots
            );
            general_constraints.push_back(sat);
        }
        for (int lix = 1; lix < lessons.size(); ++lix) {
            auto &ld = lessons[lix];
            if ((a.tag.isEmpty() || ld.tags.contains(a.tag))
                && (!a.tid || (std::find(
                    ld.teachers.begin(),
                    ld.teachers.end(), t2i.value(a.tid)) != ld.teachers.end()))
                && (!a.gid ||
                        // Test all atomic groups included in a.gid
                        [&]() -> bool {
                            for (const auto &sg : g2sg.value(a.gid)) {
                                int agi = sg2i.value(sg);
                                if (std::find(ld.atomic_groups.begin(),
                                    ld.atomic_groups.end(),
                                    agi) != ld.atomic_groups.end()) return true;
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
                        slot_constraint ttslots(ndays);
                        int d = 0;
                        for (int d = 0; d < ndays; ++d) {
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
                    merge_slot_constraints(ld, a.ttslots);
                } else {
                    // Soft constraint
                    sat->add_lesson_id(this, ld.lesson_id);
                }
            }
        }
    }
}

// Initial placement of the lessons. Call this only once!
void BasicConstraints::initial_place_lessons()
{
    if (!lessons.empty()) {
        qFatal() << "BasicConstraints::initial_place_lessons() called twice";
    }
    lessons.push_back({});  // dummy lesson at index 0
    // Place fixed lessons first.
    for (int cid : db_data->Tables.value("COURSES")) {
        LessonData ldc; // lesson data for the course
        auto node = db_data->Nodes.value(cid);
        ldc.subject = node.value("SUBJECT").toInt();
        auto glist = node.value("GROUPS").toArray();
        for(auto g : glist) {
            for (const auto &sg : g2sg.value(g.toInt())) {
                ldc.atomic_groups.push_back(sg2i.value(sg));
            }
        }
        auto tlist = node.value("TEACHERS").toArray();
        for (auto t : tlist) {
            ldc.teachers.push_back(t2i.value(t.toInt()));
        }
        const auto frarray = node.value("FIXED_ROOMS").toArray();
        for (const auto &r : frarray) {
            ldc.fixed_rooms.push_back(r2i.value(r.toInt()));
        }
        // If there is a "FLEXIBLE_ROOM", it is specific to each lesson and
        // only valid if there is a slot placement.
        for (int lid : db_data->course_lessons.value(cid)) {
            LessonData ld(ldc);
            auto lnode = db_data->Nodes.value(lid);
            ld.lesson_id = lid;
            int l = lnode.value("LENGTH").toInt();
            ld.length = l;
            const auto &atlist = lnode.value("ACTIVITY_TAGS").toArray();
            for (const auto & atj : atlist) {
                ld.tags.append(atj.toString());
            }
            int lix = lessons.size();
            lid2lix[lid] = lix;
            int d0 = lnode.value("DAY").toInt();
            if (d0 == 0) { // no placement
                ld.day = -1;
                lessons.push_back(ld);
                continue;
            }
            // Deal with lessons with a placement time
            int d = db_data->days.value(d0);
            ld.day = d;
            int h = db_data->hours.value(lnode.value("HOUR").toInt());
            ld.hour = h;
            QJsonValue cr = lnode.value("FLEXIBLE_ROOM");
            if (!cr.isUndefined()) {
                ld.flexible_room = r2i.value(cr.toInt());
            }
            // I need to place the fixed lessons first, in order to build
            // available-slot lists for each (non-fixed) lesson.
            lessons.push_back(ld);
            if (lnode.value("FIXED").toBool()) {
                place_fixed_lesson(lix);
            }
        }
    }
}

void BasicConstraints::place_fixed_lesson(int lesson_index)
{
    // To modify the lesson, a reference must now be used.
    auto &ldp = lessons[lesson_index];
    ldp.fixed = true;
    // Test placement before actually doing it. This only checks the
    // most basic criteria, i.e. clashes. Other hard constraints are
    // ignored for fixed placements.
    if (!test_possible_place(ldp, ldp.day, ldp.hour)) {
        qFatal() << "Couldn't place lesson" << ldp.lesson_id
                 << "@ Slot" << ldp.day << ldp.hour;
    }
    // Now do the placement
    for (int i = 0; i < ldp.length; i++) {
        int hh = ldp.hour + i;
        for (int t : ldp.teachers) {
            t_weeks.at(t).at(ldp.day).at(hh) = lesson_index;
        }
        for (int sg : ldp.atomic_groups) {
            sg_weeks.at(sg).at(ldp.day).at(hh) = lesson_index;
        }
        for (int r : ldp.fixed_rooms) {
            r_weeks.at(r).at(ldp.day).at(hh) = lesson_index;
        }
        if (ldp.flexible_room >= 0) {
            r_weeks.at(ldp.flexible_room).at(ldp.day).at(hh) = lesson_index;
        }
    }

}

void BasicConstraints::setup_parallels(slot_constraint &parallels)
{
    for (const std::vector<int> &plist : parallels) {
        // Need to "unite" the start_cells structures of hard-parallel lessons.
        for (int lix : plist) {
            auto &l = lessons[lix];
            if (!l.parallel_lessons.empty()) {
                qFatal() << "Lesson" << l.lesson_id
                         << "has multiple 'SameStartingTime' constraints";
            }
            for (int lix2 : plist) {
                if (lix2 != lix) {
                    l.parallel_lessons.push_back(lix2);
                    auto &l2 = lessons[lix2];
                    if (l2.fixed) {
                        if (l.fixed) qFatal() << "Two fixed lessons are"
                                     << "bound by hard"
                                     << "Same-Starting-Time constraint:"
                                     << l.lesson_id << "&" << l2.lesson_id;
//TODO: Allow it if the times are the same?

                        l.day = l2.day;
                        l.hour = l2.hour;
                        place_fixed_lesson(lix);
                    }
                    // If a start_cells array has been found, merge into that
                    // of the current lesson (l).
                    if (l2.slot_constraint_index != 0) {
                        auto &sc2 = start_cells_list.at(
                            l2.slot_constraint_index);
                        merge_slot_constraints(l, sc2);
                        sc2.clear(); // no longer needed
                    }
                    // Share constraint_slot structure
                    l2.slot_constraint_index = l.slot_constraint_index;
                }
            }
        }
    }
}

// Go through all lessons, placing the non-fixed ones which have a start-
// time. The fixed ones have already been placed.
void BasicConstraints::initial_place_lessons2(time_constraints &tconstraints)
{
    // Parallel lessons need to share start_cells.
    setup_parallels(tconstraints.parallel_lessons);
    // Include further restrictions on starting times, from constraints
    // concerning multiple activities
    multi_slot_constraints(
        tconstraints.activities_starting_times,
        false
    );
    multi_slot_constraints(
        tconstraints.activities_slots,
        true
    );    
    // Find all possible placements taking blocked cells and already
    // placed (fixed) lessons into account.
    for (int lix = 1; lix < lessons.size(); ++lix) {
        auto &ldata = lessons.at(lix);
        if (ldata.fixed) continue;

//TODO: Can this be placed in the previous loop?
        // Remove start-cells if there are day clashes with fixed lessons.
        filter_day_slots(*ldata.start_cells, lix);

        // Place the non-fixed lessons with a placement time.

//TODO: This should be done when all the lessons have been filtered!
        // Check placement (if any) and place the lesson.
        int d = ldata.day;
        if (d < 0) continue;
        int h = ldata.hour;
        // Test placement before actually doing it
        if (!test_place(ldata, d, h)) {
            qDebug() << "§1" << ldata.start_cells;

            qFatal() << "Couldn't place lesson" << ldata.lesson_id
                     << "@ Slot" << d << h;
        }
        // Now do the placement

//TODO: What about parallel rooms?!
// If one is placed here, it should not be placed again, when it is
// reached in the loop!

        for (int i = 0; i < ldata.length; i++) {
            int hh = h + i;
            for (int t : ldata.teachers) {
                t_weeks.at(t).at(d).at(hh) = lix;
            }
            for (int sg : ldata.atomic_groups) {
                sg_weeks.at(sg).at(d).at(hh) = lix;
            }
            for (int r : ldata.fixed_rooms) {
                r_weeks.at(r).at(d).at(hh) = lix;
            }
            if (ldata.flexible_room >= 0) {
                r_weeks.at(ldata.flexible_room).at(d).at(hh) = lix;
            }
        }
    }
}

// This is a primitive test for a placement. It returns only true or false,
// according to whether the placement is possible. It doesn't change
// anything.
bool BasicConstraints::test_single_slot(
    LessonData &ldata, int day, int hour)
{
    for (int i : ldata.atomic_groups) {
        if (sg_weeks[i][day][hour]) return false;
    }
    for (int i : ldata.teachers) {
        if (t_weeks[i][day][hour]) return false;
    }
    for (int i : ldata.fixed_rooms) {
        if (r_weeks[i][day][hour]) return false;
    }
    return true;
}

// Test whether the given lesson is blocked at the given time (which is
// permitted by the start_cells table). This checks all slots covered by
// the lesson.
bool BasicConstraints::test_possible_place(
    LessonData &ldata, int day, int hour)
{
    for (int lx = 0; lx < ldata.length; ++lx) {
        if (!test_single_slot(ldata, day, hour+lx)) return false;
    }
    return true;
}

// Test whether the given lesson can be placed at the given time, taking
// the permissible start times for the lesson into account.
bool BasicConstraints::test_place(LessonData &ldata, int day, int hour)
{
    const auto & dvec = (*ldata.start_cells)[day];
    for (int h : dvec) {
        if (h < hour) continue;
        if (h > hour) break;
        return test_possible_place(ldata, day, hour);
    }
    return false;
}

// The basic array of potentially available slots is passed by reference as
// start_slots. Note that this can be modified, so it should normally not be
// the start_cells array of the lesson. It could be a copy.
void BasicConstraints::filter_day_slots(
    slot_constraint &start_slots,
    int lesson_index)
{
    found_slots.clear(); // doesn't reduce the capacity
    LessonData &ldata = lessons[lesson_index];
    // Filter days using day-constraints. This cannot be handled entirely by
    // the presetting of the lesson's start_cells array because the blocked
    // days depend on non-fixed (as well as fixed) lessons, which makes them
    // rather dynamic.
    for (const auto c : ldata.day_constraints) {
        for (int d = 0; d < ndays; ++d) {
            std::vector<int> &ssd = start_slots[d];
            if (!ssd.empty()) {
                if (!c->test(this, lesson_index, d)) {
                    start_slots[d].clear();
                }
            }
        }
    }
    // Now the parallel lessons, if any.
    for (int lix : ldata.parallel_lessons) {
        LessonData &ld = lessons[lix];
        for (const auto c : ld.day_constraints) {
            for (int d = 0; d < ndays; ++d) {
                if (!start_slots[d].empty()) {
                    if (!c->test(this, lix, d)) {
                        start_slots[d].clear();
                    }
                }
            }
        }
    }
}


// Find slots (day, hour) where the given lesson can be placed.
// The basic array of potentially available slots is passed by reference as
// start_slots.
// The resulting list of slots is returned in the member variable found_slots.
// This is in order to avoid unnecessary memory management.

//TODO
void BasicConstraints::find_slots(
    slot_constraint &start_slots,
    int lesson_index)
{    
    found_slots.clear(); // doesn't reduce the capacity
    LessonData &ldata = lessons[lesson_index];
    // Now test the possible slots
    for (int d = 0; d < ndays; ++d) {
        std::vector<int> &ssd = start_slots[d];
        for (int h : ssd) {
            if (test_possible_place(ldata, d, h)) {
                found_slots.push_back({d, h});
            }
        }
    }
    // Also for the parallel lessons, if any.
    for (int lix : ldata.parallel_lessons) {
        LessonData &ld = lessons[lix];
        for (int d = 0; d < ndays; ++d) {
            std::vector<int> &ssd = start_slots[d];
            for (int h : ssd) {
                if (test_possible_place(ldata, d, h)) {
                    found_slots.push_back({d, h});
                }
            }
        }
    }
}




//TODO: Add a test which returns details of the clashes, at least the
// lessons/courses, maybe also the associated groups/teachers/rooms ...
// That could be triggered by shift-click (which would place if ok?).

// Return a list of clashing-lesson-ids.
// This seems to take 10-20 times as long as the simple test above!
// So use it only when the details are needed.
std::vector<int> BasicConstraints::find_clashes(
    LessonData *ldata, int day, int hour)
{
    std::vector<int> clashes;
    for (int i : ldata->atomic_groups) {
        int lix = sg_weeks[i][day][hour];
        if (lix && std::find(
                clashes.begin(), clashes.end(), lix) == clashes.end()) {
            clashes.push_back(lix);
        }
    }
    for (int i : ldata->teachers) {
        int lix = t_weeks[i][day][hour];
        if (lix && std::find(
                clashes.begin(), clashes.end(), lix) == clashes.end()) {
            clashes.push_back(lix);
        }
    }
    for (int i : ldata->fixed_rooms) {
        int lix = r_weeks[i][day][hour];
        if (lix && std::find(
                clashes.begin(), clashes.end(), lix) == clashes.end()) {
            clashes.push_back(lix);
        }
    }
    return clashes;
}


SameStartingTime::SameStartingTime(std::vector<int> &l_indexes, int weight)
{
    penalty = weight;
    lesson_indexes = l_indexes;
//TODO--
//    qDebug() << "SameStartingTime" << penalty << lesson_indexes;
}

//TODO
int SameStartingTime::evaluate(BasicConstraints *constraint_data) { return 0; }

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
