#include "basicconstraints.h"
#include <QJsonArray>

BasicConstraints::BasicConstraints(DBData *dbdata) : db_data{dbdata}
{
    // Each "resource" – (atomic) student group, teacher and room –
    // has a vector of slots for one week, organized as days * hours.
    // When the resource is "busy" in a time slot, the slot will contain
    // the index of the lesson-node.

    // Collect the atomic subgroups
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
                    ld.soft_constraints.push_back(sat);
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
//TODO!
        // Get the room specification, which can contain a list of necessary
        // rooms, "FIXED_ROOMS", and a list of rooms from which one is to be
        // chosen, "ROOM_CHOICE".

        for (const auto &r : node.value("FIXED_ROOMS").toArray()) {
            ldc.fixed_rooms.push_back(r2i.value(r.toInt()));
        }
        //TODO: Is something like this better?:
        //QJsonArray rarray{node.value("FIXED_ROOMS").toArray()};
        //for (auto r : std::as_const(rarray))

        /* TODO -> soft constraint?
        for (const auto &r : node.value("ROOM_CHOICE").toArray()) {
            ldc.rooms_choice.push_back(r2i.value(r.toInt()));
        }
        */

        // The occupied rooms are associated with the individual lessons.
        // Note that they are only valid if there is a slot placement.
        for (int lid : db_data->course_lessons.value(cid)) {
            LessonData ld(ldc);
            auto lnode = db_data->Nodes.value(lid);
//TODO; flexible_room
//            auto rlist = lnode.value("ROOMS").toArray();
//            for (auto r : rlist) {
//                ld.rooms.push_back(r2i.value(r.toInt()));
//            }

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
            bool fixed = lnode.value("FIXED").toBool();
            // I need to place the fixed lessons first, in order to build
            // available-slot lists for each (non-fixed) lesson.
            if (!fixed) {
                to_place.push_back(lix);
                lessons.push_back(ld);
                continue;
            }
            ld.fixed = true;
            QJsonValue cr = lnode.value("FLEXIBLE_ROOM");
            if (!cr.isUndefined()) {
                ld.flexible_room = cr.toInt();
            }
            lessons.push_back(ld);
            // Test placement before actually doing it. This only checks the
            // most basic criteria, i.e. clashes. Other hard constraints are
            // ignored for fixed placements.
            if (!test_possible_place(ld, d, h)) {
                qFatal() << "Couldn't place lesson" << lid
                         << "@ Slot" << d << h;
            }
            // Now do the placement
            for (int i = 0; i < l; i++) {
                int hh = h + i;
                for (int t : ld.teachers) {
                    t_weeks.at(t).at(d).at(hh) = lix;
                }
                for (int sg : ld.atomic_groups) {
                    sg_weeks.at(sg).at(d).at(hh) = lix;
                }
                for (int r : ld.fixed_rooms) {
                    r_weeks.at(r).at(d).at(hh) = lix;
                }
                if (ld.flexible_room >= 0) {
                    r_weeks.at(ld.flexible_room).at(d).at(hh) = lix;
                }
            }
        }
    }
    return to_place;
}

void BasicConstraints::initial_place_lessons2(
    std::vector<int> to_place, time_constraints &tconstraints)
{
    // Deal with the unfixed lessons. These need available-slot lists
    // and those with a placement time need to be placed.
    // First collect the (currently) available slots – before actually
    // placing any of the non-fixed lessons.
    for (int lix : to_place) {
        auto &ld = lessons[lix];
        // Build basic starting-slots lists for a given lesson based on lesson
        // length and hard local starting time / slot constraints.
        // As existing placed lessons should be taken into account, this
        // is done before using the method find_possile_places.
        if (!ld.start_cells.empty()) {
            // Only relevant for hard constraint:
            qFatal() << "BasicConstraints::initial_place_lessons2 called with"
                     << "non-empty start_cells";
        }
        // If there is a set of hard preferred starting times for the
        // activity, take these as the basis
        if (tconstraints.lesson_starting_times.contains(ld.lesson_id)) {
            auto lst = tconstraints.lesson_starting_times.at(ld.lesson_id);
            if (lst.isHard()) {
                ld.start_cells.resize(ndays);
                for (int d = 0; d < ndays; ++d) {
                    ld.start_cells[d] = lst.ttslots[d];
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
                ld.soft_constraints.push_back(sat);
            }
        }
        if (ld.start_cells.empty()) {
            // Otherwise initially allow all slots as starting time
            ld.start_cells.resize(ndays);
            for (auto &dvec : ld.start_cells) {
                dvec.resize(nhours);
                for (int h = 0; h < nhours; ++h) dvec[h] = h;
            }
        }
    }
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
        ldata.start_cells = find_possible_places(ldata);
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
            for (int r : ldata.rooms) {
                r_weeks.at(r).at(d).at(hh) = lix;
            }
        }
    }
}

// Test whether the given lesson is blocked at the given time (which is
// permitted by the start_cells table).
bool BasicConstraints::test_possible_place(
    LessonData &ldata, int day, int hour)
{
    for (int lx = 0; lx < ldata.length; ++lx) {
        for (int i : ldata.atomic_groups) {
            if (sg_weeks[i][day][hour+lx]) return false;
        }
        for (int i : ldata.teachers) {
            if (t_weeks[i][day][hour+lx]) return false;
        }
        // ldata->rooms is not relevant here
        for (int i : ldata.rooms_needed) {
            if (r_weeks[i][day][hour+lx]) return false;
        }
        if (ldata.rooms_choice.empty()) goto next;
        for (int i : ldata.rooms_choice) {
            if (!r_weeks[i][day][hour+lx]) goto next;
        }
        return false;
    next:;
    }
    return true;
}

// Test whether the given lesson can be placed at the given time.
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
    // ldata->rooms is not relevant here
    for (int i : ldata.rooms_needed) {
        if (r_weeks[i][day][hour]) return false;
    }
    if (ldata.rooms_choice.empty()) return true;
    for (int i : ldata.rooms_choice) {
        if (!r_weeks[i][day][hour]) return true;
    }
    return false;
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
    for (int i : ldata->rooms_needed) {
        int lix = r_weeks[i][day][hour];
        if (lix && std::find(
                clashes.begin(), clashes.end(), lix) == clashes.end()) {
            clashes.push_back(lix);
        }
    }
    if (!ldata->rooms_choice.empty()) {
        int lix;
        for (int i : ldata->rooms_choice) {
            lix = r_weeks[i][day][hour];
            if (lix && std::find(
                    clashes.begin(), clashes.end(), lix) != clashes.end()) {
                // The room is attached to a lesson which already clashes
                return clashes;
            }
        }
        // Take the last room's lesson as the clash, just so that
        // something gets listed (TODO: is there a better way?)
        clashes.push_back(lix);
    }
    return clashes;
}


SameStartingTime::SameStartingTime(
    BasicConstraints *constraint_data,
    QJsonObject node)// : Constraint()
{
    penalty = node.value("WEIGHT").toInt();
    auto llist = node.value("LESSONS").toArray();
    int n = llist.size();
    lesson_indexes.resize(n);
    for (int i = 0; i < n; ++i) {
        lesson_indexes[i] = constraint_data->lid2lix[llist[i].toInt()];
    }
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
    bool allslots)
        : Constraint(), all_slots{allslots}
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
