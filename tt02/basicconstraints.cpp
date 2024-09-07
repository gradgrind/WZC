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
        auto node = dbdata->Nodes.value(gid).DATA;
        auto sglist = node.value("SUBGROUPS").toArray();
        bool allsg = node.value("ID").toString().isEmpty();
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
    //qDebug() << "sg2i:" << sg2i;

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
        auto node = dbdata->Nodes.value(rid).DATA;
        if (node.contains("ROOMS_NEEDED")) continue;
        // A real room
        r2i[rid] = i_r.length();
        i_r.append(rid);
    }
    r_weeks = std::vector<std::vector<std::vector<int>>>(
        i_r.length(), std::vector<std::vector<int>> (
            ndays , std::vector<int> (nhours)));

    slot_blockers();
//TODO: I should perhaps handle the hard local constraints here too
// (before placing any lessons, to capture errors).
    time_constraints tconstraints = activity_slot_constraints();
    initial_place_lessons(tconstraints);
}

void BasicConstraints::slot_blockers()
{
    // Block slots where teachers are "not available"
    for (int tid : db_data->Tables.value("TEACHERS")) {
        auto node = db_data->Nodes.value(tid).DATA;
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
        auto node = db_data->Nodes.value(gid).DATA;
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
        auto node = db_data->Nodes.value(rid).DATA;
        if (node.contains("ROOMS_NEEDED")) continue;
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

// Collect Activity slot constraints
time_constraints BasicConstraints::activity_slot_constraints()
{
    time_constraints constraints;
    for (int xid : db_data->Tables.value("LOCAL_CONSTRAINTS")) {
        auto node = db_data->Nodes.value(xid).DATA;
        if (node.value("WEIGHT") != "+") continue; // only hard constraints

        auto ntype = node.value("TYPE");
        if (node.contains("SLOTS")) {
            //NOTE: I assume the times are sorted in the SLOTS list.
            std::vector<std::vector<int>> days(ndays);
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
        } else {
            // "SAME_STARTING_TIME"
            // "ONE_DAY_BETWEEN"
            // "DAYS_BETWEEN"
        }
    }
    return constraints;
}


void BasicConstraints::with_slots(
    std::vector<ActivitySelectionSlots> &alist,
    lesson_data &ld,
    bool starting_time)
{
    for (auto &a : alist) {
        if ((a.tag.isEmpty() || ld.tags.contains(a.tag))
            && (!a.tid || (std::find(ld.teachers.begin(),
                ld.teachers.end(), a.tid) != ld.teachers.end()))
            && (!a.gid || (std::find(ld.groups.begin(),
                ld.groups.end(), a.gid) != ld.groups.end()))
            && (!a.sid || a.sid == ld.subject)
            && (!a.l || a.l == ld.length)) {

            // If they are starting times, simply take them on, unless
            // there are already starting times for this lesson, then
            // build the intersection.
            // If they are available slots, the possible starting times
            // need to be built, based on the lesson length.
            // So start with that ...

            if (!starting_time && ld.length > 1) {
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
                ld.start_cells = a.ttslots;
            } else {
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
            }
        }
    }
}

// Find slots lists for a given lesson
void BasicConstraints::find_slots(
    time_constraints &constraints, lesson_data &ld)
{
    if (constraints.lesson_starting_times.contains(ld.lesson_id)) {
        ld.start_cells = constraints.lesson_starting_times[ld.lesson_id];
    }
    with_slots(constraints.activities_starting_times, ld, true);
    with_slots(constraints.activities_slots, ld, false);
}

// Initial placement of the lessons. Call this only once!
void BasicConstraints::initial_place_lessons(time_constraints &tconstraints)
{
    if (!lessons.empty()) {
        qFatal() << "BasicConstraints::initial_place_lessons() called twice";
    }
    lessons.push_back({});  // dummy lesson at index 0
    std::vector<int> to_place;
    for (int cid : db_data->Tables.value("COURSES")) {
        lesson_data ldc;
        auto node = db_data->Nodes.value(cid).DATA;
        ldc.subject = node.value("SUBJECT").toInt();
        auto glist = node.value("STUDENTS").toArray();
        for(auto g : glist) {
            for (const auto &sg : g2sg.value(g.toInt())) {
                ldc.groups.push_back(sg2i.value(sg));
            }
        }
        auto tlist = node.value("TEACHERS").toArray();
        for (auto t : tlist) {
            ldc.teachers.push_back(t2i.value(t.toInt()));
        }
        // Get the possible-rooms list, which can contain a virtual room
        // (as only member – checked in "readspaceconstraints").
        // The input is a simple list.
        auto rlist = node.value("ROOMSPEC").toArray();
        std::vector<int> rvec;
        for (auto rv : rlist) {
            int rid = rv.toInt();
            auto node = db_data->Nodes.value(rid).DATA;
            if (node.contains("ROOMS_NEEDED")) {
                // Virtual room
                auto srl = node.value("ROOMS_NEEDED").toArray();
                for (auto sr : std::as_const(srl)) {
                    ldc.rooms_needed.push_back(r2i.value(sr.toInt()));
                }
                srl = node.value("ROOMS_CHOICE").toArray();
                for (auto sr : std::as_const(srl)) {
                    rvec.push_back(r2i.value(sr.toInt()));
                }
            } else {
                // A real room
                rvec.push_back(r2i.value(rid));
            }
        }
        if (rvec.size() > 1) {
            ldc.rooms_choice = rvec;
        } else if (!rvec.empty()) {
            // Add to compulsory room list
            ldc.rooms_needed.push_back(rvec[0]);
        }

        // The used rooms are associated with the individual lessons.
        // Note that they are only valid if there is a slot placement.
        for (int lid : db_data->course_lessons.value(cid)) {
            lesson_data ld(ldc);
            auto lnode = db_data->Nodes.value(lid).DATA;
            auto rlist = lnode.value("ROOMS").toArray();
            for (auto r : rlist) {
                ld.rooms.push_back(r2i.value(r.toInt()));
            }
            ld.lesson_id = lid;
            int l = lnode.value("LENGTH").toInt();
            ld.length = l;
            const auto &atlist = lnode.value("ACTIVITY_TAGS").toArray();
            for (const auto & atj : atlist) {
                ld.tags.append(atj.toString());
            }
            int lix = lessons.size();
            lid2lix[lid] = lix;
            lessons.push_back(ld);
            int d0 = lnode.value("DAY").toInt();
            if (d0 == 0) { // no placement
                lessons[lix].day = -1;
                to_place.push_back(lix);
                find_slots(tconstraints, ld);
                continue;
            }
            // Deal with lessons with a placement time
            int d = db_data->days.value(d0);
            lessons[lix].day = d;
            int h = db_data->hours.value(lnode.value("HOUR").toInt());
            lessons[lix].hour = h;
            bool fixed = lnode.value("FIXED").toBool();
            // I need to place the fixed lessons first, in order to build
            // available-slot lists for each (non-fixed) lesson.
            if (!fixed) {
                to_place.push_back(lix);
                find_slots(tconstraints, ld);
                continue;
            }
            lessons[lix].fixed = true;
            // Test placement before actually doing it
            for (int i = 0; i < l; i++) {
                int hh = h + i;
                for (int t : ld.teachers) {
                    if (t_weeks.at(t).at(d).at(hh) != 0) {
                        qFatal() << "Couldn't place lesson" << lid
                                 << "// teacher" << i_t.at(t) << "in"
                                 << t_weeks.at(t).at(d).at(hh)
                                 << "@ Slot" << d << h;
                    }
                }
                for (int sg : ld.groups) {
                    if (sg_weeks.at(sg).at(d).at(hh) != 0) {
                        qFatal() << "Couldn't place lesson" << lid
                                 << "// group in"
                                 << sg_weeks.at(sg).at(d).at(hh);
                    }
                }
                for (int r : ld.rooms) {
                    if (r_weeks.at(r).at(d).at(hh) != 0) {
                        qFatal() << "Couldn't place lesson" << lid
                                 << "// room in"
                                 << r_weeks.at(r).at(d).at(hh);
                    }
                }
            }
            // Now do the placement
            for (int i = 0; i < l; i++) {
                int hh = h + i;
                for (int t : ld.teachers) {
                    t_weeks.at(t).at(d).at(hh) = lix;
                }
                for (int sg : ld.groups) {
                    sg_weeks.at(sg).at(d).at(hh) = lix;
                }
                for (int r : ld.rooms) {
                    r_weeks.at(r).at(d).at(hh) = lix;
                }
            }
        }
    }
    // Now deal with the unfixed lessons. These need available-slot lists
    // and those with a placement time need to be placed.
    for (int lix : to_place) {
        auto free = find_places(lix);
        auto ldata = &lessons.at(lix);
        ldata->start_cells = free;
        // Check placement (if any) and place it
        int d = ldata->day;
        if (d < 0) continue;
        int h0 = ldata->hour;
        for (int h : free[d]) {
            if (h == h0) {
                // Placement ok
                for (int i = 0; i < ldata->length; i++) {
                    int hh = h + i;
                    for (int t : ldata->teachers) {
                        t_weeks.at(t).at(d).at(hh) = lix;
                    }
                    for (int sg : ldata->groups) {
                        sg_weeks.at(sg).at(d).at(hh) = lix;
                    }
                    for (int r : ldata->rooms) {
                        r_weeks.at(r).at(d).at(hh) = lix;
                    }
                }
                goto done;
            }
        }
        qFatal() << "Couldn't place lesson" << ldata->lesson_id;
    done:;
    }
}

std::vector<std::vector<int>> BasicConstraints::find_places(int lix)
{
    std::vector<std::vector<int>> free(ndays);
    auto ldata = &lessons.at(lix);
    if (ldata->length == 1) {
        for (int d = 0; d < ndays; ++d) {
            for (int h = 0; h < nhours; ++h) {
                if (test_place_lesson(ldata, d, h)) {
                    //qDebug() << "OK:" << d << h;
                    free[d].push_back(h);
                }
            }
        }
    } else {
        int l = ldata->length;
        for (int d = 0; d < ndays; ++d) {
            int count = 0;
            int h = 0;
            while (h < nhours) {
                if (test_place_lesson(ldata, d, h++)) {
                    if (++count == l) {
                        free[d].push_back(h - l);
                        --count;
                    }
                } else {
                    count = 0;
                }
            }
        }
    }
    return free;
}


// This is a primitive test for a placement. It returns only true or false,
// according to whether the placement is possible. It doesn't change
// anything.
bool BasicConstraints::test_place_lesson(
    lesson_data *ldata, int day, int hour)
{
    for (int i : ldata->groups) {
        if (sg_weeks[i][day][hour]) return false;
    }
    for (int i : ldata->teachers) {
        if (t_weeks[i][day][hour]) return false;
    }
    // ldata->rooms is not relevant here
    for (int i : ldata->rooms_needed) {
        if (r_weeks[i][day][hour]) return false;
    }
    if (ldata->rooms_choice.empty()) return true;
    for (int i : ldata->rooms_choice) {
        if (!r_weeks[i][day][hour]) return true;
    }
    return false;
}
//TODO: Deal with length > 1, maybe parallel lessons?
//TODO: Add a test which returns details of the clashes, at least the
// lessons/courses, maybe also the associated groups/teachers/rooms ...
// That could be triggered by shift-click (which would place if ok?).

// Return a list of clashing-lesson-ids.
// This seems to take 10-20 times as long as the simple test above!
// So use it only when the details are needed.
std::vector<int> BasicConstraints::find_clashes(
    lesson_data *ldata, int day, int hour)
{
    std::vector<int> clashes;
    for (int i : ldata->groups) {
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
