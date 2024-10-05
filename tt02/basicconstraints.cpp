#include "basicconstraints.h"
#include <QJsonArray>
#include <unordered_set>

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
    sg_weeks = std::vector<std::vector<std::vector<int>>>(
        i_sg.length(), std::vector<std::vector<int>> (
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
    t_weeks = std::vector<std::vector<std::vector<int>>>(
        i_t.length(), std::vector<std::vector<int>> (
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
    r_weeks = std::vector<std::vector<std::vector<int>>>(
        i_r.length(), std::vector<std::vector<int>> (
            ndays , std::vector<int> (nhours)));

    // First enter the NOT_AVAILABLE constraints into the week-vectors
    // for teachers, student groups and rooms.
    slot_blockers();
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
    std::vector<int> &to_place,
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
        for (int lix : to_place) {
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
                    // If they are starting times, simply take them on board,
                    // unless there are already starting times for this lesson,
                    // then build the intersection.
                    // If they are available slots, the possible starting times
                    // need to be built, based on the lesson length.
                    // So start with that ...
                    if (allslots && ld.length > 1) {
                        std::vector<std::vector<int>> ttslots(ndays);
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
                    if (ld.start_cells.empty()) {
                        qFatal() << "multi_slot_constraints called with"
                                 << "start_cells empty, lesson"
                                 << ld.lesson_id;
                    }
                    for (int d = 0; d < ndays; ++d) {
                        const auto hvec = a.ttslots[d];
                        int hvecl = hvec.size();
                        int i = 0;
                        std::vector<int> hlist;
                        for (int h : ld.start_cells[d]) {
                            while (i < hvecl) {
                                int hh = hvec[i];
                                if (hh == h) {
                                    hlist.push_back(hh);
                                    ++i;
                                    break;
                                }
                                else if (hh > h) break;
                                ++i;
                            }
                        }
                        ld.start_cells[d] = hlist;
                    }
                } else {
                    // Soft constraint
                    sat->add_lesson_id(this, ld.lesson_id);
                }
            }
        }
    }
}

// Initial placement of the lessons. Call this only once!
std::vector<int> BasicConstraints::initial_place_lessons()
{
    if (!lessons.empty()) {
        qFatal() << "BasicConstraints::initial_place_lessons() called twice";
    }
    lessons.push_back({});  // dummy lesson at index 0
    std::vector<int> to_place; // collect non-fixed lessons for later placement
    // Place fixed lessons first.
//TODO: What to do about parallel lessons? I suppose it is possible, in
// principle, to have two fixed lessons parallel (shouldn't be allowed, though).
// Also, a non-fixed lesson may be parallel to a fixed one. It must then be
// reset as fixed in the timetable lesson structure.

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
        // The occupied rooms are associated with the individual lessons.
        // Note that they are only valid if there is a slot placement.
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
                to_place.push_back(lix);
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
            bool fixed = lnode.value("FIXED").toBool();
            // I need to place the fixed lessons first, in order to build
            // available-slot lists for each (non-fixed) lesson.
            lessons.push_back(ld);
            if (fixed) {
                place_fixed_lesson(lix);
            } else {
                to_place.push_back(lix);
            }
        }
    }
    return to_place;
}

void BasicConstraints::place_fixed_lesson(int lesson_index)
{
    // To modify the lesson, a reference must now be used (ld
    // is not the lesson in "lessons".
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

void BasicConstraints::initial_place_lessons2(
    std::vector<int> to_place, time_constraints &tconstraints)
{
//TODO: Parallel lessons need to have identical start_cells!
    std::unordered_set<int> done_parallels;
    // Deal with the unfixed lessons. These need available-slot lists
    // and those with a placement time need to be placed.
    // First collect the (currently) available slots – before actually
    // placing any of the non-fixed lessons.
    for (int lix : to_place) {
        auto &ld = lessons[lix];
        // Build basic starting-slots lists for a given lesson based on lesson
        // length and hard local starting time / slot constraints.
        // As existing placed lessons should be taken into account, this
        // is done before placing these lessons.
        if (!ld.start_cells.empty()) {
            // Only relevant for hard constraint:
            qFatal() << "BasicConstraints::initial_place_lessons2 called with"
                     << "non-empty start_cells";
        }
        std::vector<std::vector<bool>> start_times(ndays,
            std::vector<bool>(nhours, true));
        // If there is a set of hard preferred starting times for the
        // activity, include these. Consider all parallel lessons.
        if (ld.parallel && !done_parallels.contains(lix)) {
            if (!done_parallels.contains(lix)) {
                for (int l_ix : ld.parallel->lesson_indexes) {
                    set_times(
                        start_times, tconstraints.lesson_starting_times, l_ix);
                    done_parallels.insert(l_ix);
                }
            }
        } else {
            set_times(
                start_times, tconstraints.lesson_starting_times, lix);
        }
    }

//TODO: Need to modify these to use start_times and at the end set start_cells.
    // Does that at all work???

    // What about shared pointers for the starting times, so that they can
    // be really shared?

    // Include further restrictions on starting times, from constraints
    // concerning multiple activities
    multi_slot_constraints(
        tconstraints.activities_starting_times,
        to_place,
        false
    );
    multi_slot_constraints(
        tconstraints.activities_slots,
        to_place,
        true
    );

    for (int lix : to_place) {
        auto &ldata = lessons.at(lix);
        // Find all possible placements taking blocked cells and already
        // placed lessons into account.

        //        ldata.start_cells = find_possible_places(ldata);
//TODO: Use find_slots?
        const auto sarray = find_slots(lix);

        ldata.start_cells.resize(ndays, {});

        for (int d = 0; d < sarray.size(); ++d) {
            auto &dp = ldata.start_cells[d];
            for (int h : sarray[d]) {
                dp.push_back(h);
            }
        }


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


// ##########################

void BasicConstraints::set_times(
    std::vector<std::vector<bool>> &slotflags,
    std::unordered_map<int, LessonStartingSlots> &starting_times,
    int lix)
{
    auto &ld = lessons[lix];
    if (starting_times.contains(ld.lesson_id)) {
        auto lst = starting_times.at(ld.lesson_id);
        if (lst.isHard()) {
            for (int d = 0; d < ndays; ++d) {
                std::vector<bool> &st = slotflags[d];
                std::vector<int> &h_ok = lst.ttslots[d];
                int i = 0;
                int ok;
                if (h_ok.empty()) {
                    ok = -1;
                } else {
                    ok = h_ok[0];
                }
                for (int h = 0; h < nhours; ++h) {
                    if (h == ok) {
                        // Get next ok value
                        ++i;
                        if (i < h_ok.size()) {
                            ok = h_ok[i];
                        } else {
                            ok = -1;
                        }
                    } else {
                        st[h] = false;
                    }
                }
            }
        } else {
            // Add soft constraint reference
            auto sat = new SoftActivityTimes(this,
                lst.weight,
                lst.ttslots,
                false
            );
            sat->add_lesson_id(this, ld.lesson_id);
            general_constraints.push_back(sat);
        }
    }
}
// ##########################


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
    const auto & dvec = ldata.start_cells[day];
    for (int h : dvec) {
        if (h < hour) continue;
        if (h > hour) break;
        return test_possible_place(ldata, day, hour);
    }
    return false;
}

std::vector<std::vector<int>> BasicConstraints::find_possible_places(
    LessonData &ldata)
{
    std::vector<std::vector<int>> free(ndays);
    if (ldata.length == 1) {
        for (int d = 0; d < ndays; ++d) {
            const auto & dvec = ldata.start_cells[d];
            for (int h : dvec) {
                if (test_single_slot(ldata, d, h)) {
                    free[d].push_back(h);
                }
            }
        }
    } else {
        int l = ldata.length;
        for (int d = 0; d < ndays; ++d) {
            const auto & dvec = ldata.start_cells[d];
            int hmax = 0;
            for (int h : dvec) {
                if (hmax < h) hmax = h;
                while (test_single_slot(ldata, d, hmax)) {
                    // Overrunning hmax should not be possible as the length
                    // is taken into consideration when building start_cells.
                    if (++hmax - h == l) {
                        free[d].push_back(h);
                        break;
                    }
                }
                // If testing of hmax failed and it is in the range of the
                // next h value, the test can be repeated. To avoid that it
                // would be necessary to save the failed hmax and test for
                // it later. It is not clear that this would be a great
                // improvement.
            }
        }
    }
    return free;
}

std::vector<std::vector<int>> BasicConstraints::find_slots(int lesson_index)
{
    LessonData &ldata = lessons[lesson_index];
    std::vector<std::vector<int>> start_slots{ldata.start_cells};
    if (!ldata.parallel) {
        for (const auto c : ldata.day_constraints) {
            for (int d = 0; d < ndays; ++d) {
                if (!start_slots[d].empty()) {
                    if (!c->test(this, lesson_index, d)) {
                        start_slots[d].clear();
                    }
                }
            }
        }
        for (int d = 0; d < ndays; ++d) {
            std::vector<int> &ssd = start_slots[d];
            for (int i = 0; i < ssd.size(); ++i) {
                int h = ssd[i];
                if (h < 0) continue;
                if (!test_possible_place(ldata, d, h)) {
                    ssd[i] = -1; // mark as invalid
                }
            }
        }
        return start_slots;
    }
    for (int lix : ldata.parallel->lesson_indexes) {
        // Filter days using day-constraints
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
        if (lix == lesson_index) continue;
        // Filter using start-cells
        std::vector<std::vector<int>> &ss1 = ld.start_cells;
        for (int d = 0; d < start_slots.size(); ++d) {
            std::vector<int> &hl0 = start_slots[d];
            if (hl0.empty()) continue;
            std::vector<int> &hl1 = ss1[d];
            int i = 0;
            for (int j = 0; j < hl0.size(); ++j) {
                int h = hl0[j];
                while (i < hl1.size()) {
                    if (hl1[i] == h) goto keep;
                    if (hl1[i] > h) break;
                    ++i;
                }
                hl0[j] = -1; // mark as invalid
            keep:;
            }
        }
    }
    for (int lix : ldata.parallel->lesson_indexes) {
        // Test theoretically available slots
        LessonData &ld = lessons[lix];
        for (int d = 0; d < ndays; ++d) {
            std::vector<int> &ssd = start_slots[d];
            for (int i = 0; i < ssd.size(); ++i) {
                int h = ssd[i];
                if (h < 0) continue;
                if (!test_possible_place(ld, d, h)) {
                    ssd[i] = -1; // mark as invalid
                }
            }
        }
    }
    return start_slots;
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
    std::vector<std::vector<int>> &ttslots,
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
