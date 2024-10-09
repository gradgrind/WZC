#include "basicconstraints.h"
#include <QJsonArray>

// Apply a filter to a set of permitted slots. The filter array is of the same
// form as the original structure, a vector of integer-vectors containing the
// permitted hours (periods) for each day. The filter array may, however,
// contain negative hour values, which are simply ignored.
// The original is modified in-place.
void restrict_week_slots(
    std::vector<std::vector<int>> &weekslots,
    const std::vector<std::vector<int>> &newslots)
{
    if (weekslots.empty()) {
        weekslots = newslots;
        return;
    }
    for (int d = 0; d < weekslots.size(); ++d) {
        auto &dayslots = weekslots[d];
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

void BasicConstraints::set_start_cells_array(
    int lid, std::vector<std::vector<int>> &week_slots)
{
    int lix = lid2lix.at(lid);
    if (start_cells_arrays.contains(lix)) {
        qFatal() << "Multiple starting times constraints for"
                 << "lesson id" << lid;
    }
    start_cells_arrays[lix] = week_slots;
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
                    // Merge in new start-slots
                    restrict_week_slots(*ld.start_cells, a.ttslots);
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
            ld.fixed = lnode.value("FIXED").toBool();
            // I need to place the fixed lessons first, in order to build
            // available-slot lists for each (non-fixed) lesson.
            lessons.push_back(ld);
            if (ld.fixed) {
                place_fixed_lesson(lix);
            }
        }
    }
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

// Construct a non-restrictive start-cells array for the given lesson-index.
// The array is placed in BasicConstraints::start_cells_arrays
// (key is the given index). The reference to this is returned as result, but
// not placed in the lesson's start_cells variable.
std::vector<std::vector<int>> * BasicConstraints::defaultStartCells(
    int lesson_index)
{
    // A non-restrictive start_cells array is needed.
    std::vector<std::vector<int>> sc0(
        ndays, std::vector<int>(nhours));
    for (int d = 0; d < ndays; ++d) {
        for (int h = 0; h < nhours; ++h) {
            sc0[d][h] = h;
        }
    }
    start_cells_arrays[lesson_index] = sc0;
    return &start_cells_arrays[lesson_index];
}

void BasicConstraints::setup_parallels(std::vector<std::vector<int>> &parallels)
{
    for (const std::vector<int> &plist : parallels) {
        // Need to "unite" the start_cells arrays of hard-parallel lessons.
        std::vector<std::vector<int>> *sc0;
        for (int lix : plist) {
            auto &l = lessons[lix];
            if (!l.parallel_lessons.empty()) {
                qFatal() << "Lesson" << l.lesson_id
                         << "has multiple 'SameStartingTime' constraints";
            }
            // If a lesson has a hard lesson-specific "Preferred Starting
            // Times" constraint, that will have been processed already, i.e.
            // available for key lix in the start_cells_arrays. Only the array
            // for the first parallel lesson with a non-empty start_cells array
            // will be picked up because any entries for the other ones will
            // be erased after merging them in.
            if (start_cells_arrays.contains(lix)) {
                sc0 = &start_cells_arrays[lix];
                l.start_cells = sc0;
            }
            for (int lix2 : plist) {
                if (lix2 != lix) {
                    l.parallel_lessons.push_back(lix2);
                }
                auto &l2 = lessons[lix2];
                if (l2.fixed) {
                    if (l.fixed) qFatal() << "Two fixed lessons are"
                                 << "bound by hard"
                                 << "Same-Starting-Time constraint:"
                                 << l.lesson_id << "&" << l2.lesson_id;
                    //TODO: Allow it if the times are the same?
                    l.day = l2.day;
                    l.hour = l2.hour;
                    l.fixed = true;
                    place_fixed_lesson(lix);
                }
                // If a start_cells array has been found, merge that from
                // the current lesson and then erase the latter.
                if (sc0 && start_cells_arrays.contains(lix2)) {
                    restrict_week_slots(*sc0, start_cells_arrays[lix2]);
                    start_cells_arrays.erase(lix2);
                    l2.start_cells = sc0;
                }
            }
        }
        if (!sc0) {
            // Deal with the cases without preferred slots.
            sc0 = defaultStartCells(plist[0]);
            // Store to each component lesson.
            for (int lix : plist) {
                auto &l = lessons[lix];
                l.start_cells = sc0;
            }
        }
    }
}

//TODO: Do without to_place. Just go through all lessons, the fixed ones
// are easily recognized.
void BasicConstraints::initial_place_lessons2(time_constraints &tconstraints)
{
    // Parallel lessons need to share start_cells.
    setup_parallels(tconstraints.parallel_lessons);
    // Deal with the unfixed lessons. These need available-slot arrays
    // and those with a placement time need to be placed.
    // First collect the (currently) available slots for each lesson – before
    // actually placing any of the non-fixed lessons.
    for (int lix = 1; lix < lessons.size(); ++lix) {
        auto &ld = lessons[lix];
        // Build basic starting-slots lists for a given lesson based on lesson
        // length and hard local starting time / slot constraints.
        // As existing placed lessons should be taken into account, this step
        // is done before placing these lessons.
        if (!ld.start_cells) {
            // Parallel lessons will already have an array, all others need one.
            // Hard preferred starting times, if defined, will be in
            // start_cells_arrays.
            if (start_cells_arrays.contains(lix)) {
                ld.start_cells = &start_cells_arrays[lix];
            } else {
                ld.start_cells = defaultStartCells(lix);
            }
        }
    }
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
        const auto ok_slots = find_slots(lix);
//TODO: Isn't this the new array?

//        restrict_week_slots(*ldata.start_cells, ok_slots);

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

//TODO: Maybe I should pass in a reference to the structure to be reduced?
// If it is new, it would have to be built before calling.
std::vector<std::vector<int>> BasicConstraints::find_slots(
    std::vector<std::vector<int>> &start_slots,
    int lesson_index)
{
    LessonData &ldata = lessons[lesson_index];
    if (ldata.parallel_lessons.empty()) {
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
    for (int lix : ldata.parallel_lessons) {
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
