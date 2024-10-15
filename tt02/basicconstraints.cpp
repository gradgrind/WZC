#include "basicconstraints.h"
#include <QJsonArray>
#include <unordered_set>

void BasicConstraints::update_db_field(int id, QString field, QJsonValue val)
{
    auto &lnode = db_data->Nodes[id];
    lnode[field] = val;
}

void BasicConstraints::remove_db_field(int id, QString field)
{
    auto &lnode = db_data->Nodes[id];
    lnode.remove(field);
}

QString BasicConstraints::pr_lesson(int lix)
{
    auto &ldata = lessons.at(lix);
    auto lnode = db_data->Nodes.value(ldata.lesson_id);
    int cid = lnode.value("COURSE").toInt();
    auto cnode = db_data->Nodes.value(cid);
    //auto sid = cnode.value("SUBJECT").toInt();
    auto sbj = db_data->get_tag(ldata.subject);

    QStringList t;
    for (int tix : ldata.teachers) {
        t.append(db_data->get_tag(i_t.at(tix)));
        //qDebug() << "$$?" << tix << t2i.value(tix)
        //         << db_data->get_tag(t2i.value(tix));
    }
    auto tlist = t.join(",");

    QStringList g;
    auto groups = cnode.value("GROUPS").toArray();
    for (auto g0 : groups) {
        int gid = g0.toInt();
        auto gnode = db_data->Nodes.value(gid);
        int clid = gnode.value("CLASS").toInt();
        auto gtag = db_data->get_tag(gid);
        if (gtag.isEmpty()) g.append(db_data->get_tag(clid));
        else g.append(QString("%1.%2").arg(db_data->get_tag(clid), gtag));
    }
    auto glist = g.join(",");

    QStringList r;
    auto rooms = cnode.value("FIXED_ROOMS").toArray();
    for (auto r0 : rooms) {
        r.append(db_data->get_tag(r0.toInt()));
    }
    auto rlist = r.join(",");

    QString timeslot;
    if (ldata.day >= 0) {
        if (ldata.flexible_room >= 0) {
            rlist += "/" + db_data->get_tag(i_r.at(ldata.flexible_room));
        }
        timeslot = QString("@%1.%2").arg(ldata.day).arg(ldata.hour);
    }

    return QString("[%1]%2(%3):%4/%5%6{%7}").arg(ldata.lesson_id).arg(
        sbj, QString::number(ldata.length), glist, tlist, timeslot, rlist);
}

QString BasicConstraints::pr_week_block_sg(int ix)
{
    QStringList dlist{QString("AG %1::\n").arg(i_sg[ix])};
    auto &block = sg_weeks[ix];
    for (int d = 0; d < ndays; ++d) {
        dlist.append(QString("  Day %1:\n").arg(d));
        for (int h = 0; h < nhours; ++h) {
            int lix = block[d][h];
            if (lix < 0) dlist.append("    XX\n");
            else if (lix == 0) dlist.append("    --\n");
            else dlist.append(QString("    %1\n").arg(pr_lesson(lix)));
        }
    }
    return dlist.join("");
}

// Apply a filter to the set of permitted slots belonging to a lesson.
// The filter array is of the same form as the original structure, a
// slot_constraint item – a vector of integer-vectors containing the
// permitted hours (periods) for each day.
// The original is modified in-place if its slot_constraints index is
// non-zero, otherwise a new entry is made for the lesson with the values
// as in the modifier.
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
            rpt:;
                if (h < h0) continue;
                if (h == h0) {
                    if (i != x) dayslots[i] = h;
                    ++i;
                }
                ++x;
                if (x < dayslots.size()) {
                    h0 = dayslots[x];
                    goto rpt;
                } else {
                    // No more source hours
                    break;
                }
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

void BasicConstraints::set_different_days(std::vector<int> &lesson_ids)
// Add lesson-indexes to the different_days lists of each lesson
{
    std::vector<int> lixs(lesson_ids.size());
    for (int i = 0; i < lesson_ids.size(); ++i) {
        lixs[i] = lid2lix.at(lesson_ids[i]);
    }
    for (int lix : lixs) {
        auto &l = lessons.at(lix);
        for (int lix2 : lixs) {
            if (lix2 != lix) {
                l.different_days.push_back(lix2);
            }
        }
    }
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
    blocked_days.resize(ndays, false);

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
            if (!cr.isUndefined())
                flexirooms.push_back({lix, r2i.value(cr.toInt())});
            // I need to place the fixed lessons first, in order to build
            // available-slot lists for each (non-fixed) lesson.
            lessons.push_back(ld);
            if (lnode.value("FIXED").toBool()) {
                place_fixed_lesson(lix);
            }
        }
    }
}

void BasicConstraints::place_lesson(int lesson_index)
{
    auto &ldp = lessons[lesson_index];
    for (int i = 0; i < ldp.length; ++i) {
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
    }
}

void BasicConstraints::place_fixed_lesson(int lesson_index)
{
    auto &ldp = lessons[lesson_index];
    // Test placement before actually doing it. This only checks the
    // most basic criteria, i.e. clashes. Other hard constraints are
    // ignored for fixed placements.
    if (!test_possible_place(ldp, ldp.day, ldp.hour)) {
        qFatal() << "Couldn't place fixed lesson" << ldp.lesson_id
                 << "@ Slot" << ldp.day << ldp.hour;
    }
    ldp.fixed = true;
    // Now do the placement
    place_lesson(lesson_index);
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
    // Find all possible placements taking blocked cells and already
    // placed (fixed) lessons into account.
    repeatfor:
    for (int lix = 1; lix < lessons.size(); ++lix) {
        auto &ldata = lessons.at(lix);
        if (ldata.fixed) continue;
        find_slots(lix, false);
        if (found_slots.empty())
            qFatal() << "No timeslots are available for lesson"
                     << ldata.lesson_id;
        if (found_slots.size() == 1) {
            auto [d, h] = found_slots[0];
            // Check that the available slot is the desired one.
            if (ldata.day < 0 || (ldata.day == d && ldata.hour == h)) {
                qWarning() << "Lesson has only one available time-slot:"
                           << pr_lesson(lix) << " ... making it \"fixed\"";
                //qDebug() << "§§§ slot_constraints:" << start_cells_list.at(
                //    ldata.slot_constraint_index);
                ldata.fixed = true;
                // Now do the placement
                ldata.day = d;
                ldata.hour = h;
                place_lesson(lix);
                // Also any parallel lessons
                for (int lixp : ldata.parallel_lessons) {
                    auto &ldp = lessons.at(lixp);
                    ldp.fixed = true;
                    // Now do the placement
                    ldp.day = d;
                    ldp.hour = h;
                    place_lesson(lixp);
                }
                goto repeatfor;
            }
        }
        slot_constraint newsc(ndays);
        for (auto dh : found_slots) {
            newsc[dh.day].push_back(dh.hour);
        }
        merge_slot_constraints(ldata, newsc);
    }

    // Place the non-fixed lessons with a placement time.
    std::unordered_set<int> done_parallel;
    for (int lix = 1; lix < lessons.size(); ++lix) {
        auto &ldata = lessons.at(lix);
        if (ldata.fixed) continue;

        // Check placement (if any) and place the lesson.
        int d = ldata.day;
        if (d < 0) continue;

        // Check if it has been placed already as a parallel lesson
        if (done_parallel.contains(lix)) continue;

        int h = ldata.hour;
        // Test placement before actually doing it
        if (!test_slot(lix, d, h)) {
            ldata.day = -1;
            qFatal() << "Couldn't place lesson" << ldata.lesson_id
                       << "@ Slot" << d << h << "\n" << pr_lesson(lix);
            continue;
        }

        place_lesson(lix);
        for (int lixp : ldata.parallel_lessons) {
            place_lesson(lixp);
            done_parallel.insert(lixp);
        }
    }

    // Now deal with the flexible rooms
    for (auto [lix, rix] : flexirooms) {
        auto &ldp = lessons[lix];
        auto &rdp = r_weeks.at(rix).at(ldp.day);
        for (int i = 0; i < ldp.length; ++i) {
            int hh = ldp.hour + i;
            if (rdp.at(hh) != 0) {
                qWarning() << "Couldn't allocate flexible room"
                           << db_data->get_tag(i_r[rix])
                           << "for lesson" << pr_lesson(lix);
                // Need to update database entry for lesson
                remove_db_field(ldp.lesson_id, "FLEXIBLE_ROOM");
                goto fail;
            }
        }
        for (int i = 0; i < ldp.length; ++i) {
            int hh = ldp.hour + i;
            rdp.at(hh) = lix;
        }
        ldp.flexible_room = rix;
    fail:;
    }
}

// This is a primitive test for a placement. It returns only true or false,
// according to whether the placement is possible. It doesn't change
// anything.
// It doesn't check whether a package is clashing with itself. This is a
// reasonable way to behave in most situations, as the lesson being tested
// will generally not have a time-slot yet. This function should probably not
// be used for lessons which do have a time-slot.
bool BasicConstraints::test_single_slot(
    LessonData &ldata, int day, int hour)
{
    for (int i : ldata.atomic_groups) {
        if (sg_weeks[i][day][hour] != 0) return false;
    }
    for (int i : ldata.teachers) {
        if (t_weeks[i][day][hour] != 0) return false;
    }
    for (int i : ldata.fixed_rooms) {
        if (r_weeks[i][day][hour] != 0) return false;
    }
    return true;
}

// Test whether the given lesson is blocked at the given time (which is
// permitted by the start-slots table). This checks all slots covered by
// the lesson (i.e. taking its length into account).
bool BasicConstraints::test_possible_place(
    LessonData &ldata, int day, int hour)
{
    for (int lx = 0; lx < ldata.length; ++lx) {
        if (!test_single_slot(ldata, day, hour+lx)) return false;
    }
    return true;
}

// Find slots (day, hour) where the given lesson can be placed.
// The resulting list of slots is returned in the member variable found_slots.
// This is in order to avoid unnecessary memory management.
// If parameter "full" is not true, the relationships to other lessons
// will only be checked with "fixed" lessons (this is needed in the
// initialization process).
void BasicConstraints::find_slots(int lesson_index, bool full)
{
    LessonData &ldata = lessons[lesson_index];
    found_slots.clear(); // doesn't reduce the capacity
    // Get days blocked by different-days constraints. The result is
    // placed in blocked_days.
    for (int d = 0; d < ndays; ++d) blocked_days[d] = false;
    for (const auto lix2 : ldata.different_days) {
        auto &l2 = lessons.at(lix2);
        if (full || l2.fixed) {
            int d2 = l2.day;
            if (d2 >= 0) blocked_days[d2] = true;
        }
    }
    // Now the parallel lessons, if any.
    for (int lixp : ldata.parallel_lessons) {
        LessonData &lp = lessons[lixp];
        for (const auto lix2 : lp.different_days) {
            auto &l2 = lessons.at(lix2);
            if (full || l2.fixed) {
                int d2 = l2.day;
                if (d2 >= 0) blocked_days[d2] = true;
            }
        }
    }

    // Now test the possible slots
    auto &start_slots = start_cells_list[ldata.slot_constraint_index];
    for (int d = 0; d < ndays; ++d) {
        if (blocked_days[d]) continue;
        std::vector<int> &ssd = start_slots[d];
        for (int h : ssd) {
            if (test_possible_place(ldata, d, h)) {
                // Test parallel lessons
                for (int lixp : ldata.parallel_lessons) {
                    LessonData &ldp = lessons[lixp];
                    if (!test_possible_place(ldp, d, h)) goto fail;
                }
                found_slots.push_back({d, h});
            }
        fail:;
        }
    }
}

// Perform a full placement test for the given lesson at the given time.
bool BasicConstraints::test_slot(int lesson_index, int day, int hour)
{
    LessonData &ldata = lessons[lesson_index];
    // Get days blocked by different-days constraints.
    for (const auto lix2 : ldata.different_days) {
        auto &l2 = lessons.at(lix2);
        if (l2.day == day) {
            //qDebug() << "DIFFDAYS:" << pr_lesson(lix2);
            return false;
        }
    }
    // Now the parallel lessons, if any.
    for (int lixp : ldata.parallel_lessons) {
        LessonData &lp = lessons[lixp];
        for (const auto lix2 : lp.different_days) {
            auto &l2 = lessons.at(lix2);
            if (l2.day == day) return false;
        }
    }

    // Now test the slot
    auto &start_slots = start_cells_list[ldata.slot_constraint_index];
    std::vector<int> &ssd = start_slots[day];
    for (int h : ssd) {
        if (h < hour) continue;
        if (h == hour) {
            //qDebug() << "  test_slot" << pr_lesson(lid2lix[ldata.lesson_id]);
            if (test_possible_place(ldata, day, hour)) {
                // Test parallel lessons
                for (int lixp : ldata.parallel_lessons) {
                    LessonData &ldp = lessons[lixp];
                    if (!test_possible_place(ldp, day, hour)) return false;
                }
                return true;
            }
        }
        break;
    }
    return false;
}


//TODO: Adapt this to use find_clashes – it is then only for manual use.
// The result should distinguish between slots that are completely blocked
// (unavailable resource or clash with fixed lesson), those that can be
// occupied by replacing one or more lessons, those where the blockage is
// only caused by flexible rooms, and those without a blockage.
std::map<TTSlot, std::set<ClashItem>>
    BasicConstraints::available_slots(int lesson_index)
{
    LessonData &ldata = lessons[lesson_index];
    std::map<TTSlot, std::set<ClashItem>> result;
    // Test the possible slots
    auto &start_slots = start_cells_list[ldata.slot_constraint_index];
    for (int d = 0; d < ndays; ++d) {
        std::vector<int> &ssd = start_slots[d];
        for (int h : ssd) {
            auto clashes = find_clashes(lesson_index, d, h);
            // Test parallel lessons
            for (int lixp : ldata.parallel_lessons) {
                clashes.merge(find_clashes(lixp, d, h));
            }
            result[{d, h}] = clashes;
        }
    }
    return result;
}

// Test the possibility of placing the given lesson in the geiven time-slot,
// returning some details of the conflicts. For each conflicting lesson
// there is an entry in the returned mapping. If there is a conflict with
// a blocked time, the lesson index is -1.

// This function is designed for manual checking. The room check has a special
// value for clashes with flexible rooms (which can be replaced if the new
// lesson is placed there).
std::set<ClashItem> BasicConstraints::find_clashes(
    int lesson_index, int day, int hour)
{
    LessonData &ldata = lessons[lesson_index];
    std::set<ClashItem> clashset;
    // Get days blocked by different-days constraints.
    for (const auto lix2 : ldata.different_days) {
        auto &l2 = lessons.at(lix2);
        if (l2.day == day) clashset.insert({lix2, DIFFERENT_DAYS});
    }
    // Now the parallel lessons, if any.
    for (int lixp : ldata.parallel_lessons) {
        LessonData &lp = lessons[lixp];
        for (const auto lix2 : lp.different_days) {
            auto &l2 = lessons.at(lix2);
            if (l2.day == day) clashset.insert({lix2, DIFFERENT_DAYS});
        }
    }
    // Now test the slot
    find_clashes2(clashset, lesson_index, day, hour);
    // and the parallel lessons
    for (int lixp : ldata.parallel_lessons) {
        LessonData &ldp = lessons[lixp];
        find_clashes2(clashset, lixp, day, hour);
    }
    return clashset;
}

//TODO: Special version for automatic replacements. Only statically available
// slots should be tested, to avoid fixed lessons and otherwise blocked slots.
// Also, only seek the blocking lessons (no details).
// Don't regard flexible rooms as special cases?

void BasicConstraints::find_clashes2(
    std::set<ClashItem> &clashset, int lix, int day, int hour)
//    LessonData &ldata, int day, int hour)
{
    LessonData &ldata = lessons[lix];
    for (int ln = 0; ln < ldata.length; ++ln) {
        int h = hour + ln;
        for (int i : ldata.fixed_rooms) {
            int lixk = r_weeks[i][day][h];
            if (lixk != 0 && lixk != lix) {
                // Check whether it is only a flexible-room clash.
                // If so, add it only if it is the first clash for this lesson.
                auto &lk = lessons[lixk];
                if (lk.flexible_room == i) clashset.insert({lixk, FLEXIROOM});
                else clashset.insert({lixk, ROOM});
            }
        }
        for (int i : ldata.atomic_groups) {
            int lixk = sg_weeks[i][day][h];
            if (lixk != 0 && lixk != lix) clashset.insert({lixk, GROUP});
        }
        for (int i : ldata.teachers) {
            int lixk = t_weeks[i][day][h];
            if (lixk != 0 && lixk != lix) clashset.insert({lixk, TEACHER});
        }
    }
}
