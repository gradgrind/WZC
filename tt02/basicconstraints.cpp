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
    initial_place_lessons();
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

// Initial placement of the lessons.
void BasicConstraints::initial_place_lessons()
{
    for (int cid : db_data->Tables.value("COURSES")) {
        lesson_data ldc;
        auto node = db_data->Nodes.value(cid).DATA;
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
            lesson_resources[lid] = ld;
            int d0 = lnode.value("DAY").toInt();
            if (d0 == 0) continue; // no placement
            int d = db_data->days.value(d0);
            int h = db_data->hours.value(lnode.value("HOUR").toInt());
            int l = lnode.value("LENGTH").toInt();
            //TODO: What to do with "FIXED"?
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
                    t_weeks.at(t).at(d).at(hh) = lid;
                }
                for (int sg : ld.groups) {
                    sg_weeks.at(sg).at(d).at(hh) = lid;
                }
                for (int r : ld.rooms) {
                    r_weeks.at(r).at(d).at(hh) = lid;
                }
            }
        }
    }
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
//TODO: Deal with length > 1
//TODO: Add a test which returns details of the clashesm at least the
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
        int lid = sg_weeks[i][day][hour];
        if (lid && std::find(
                clashes.begin(), clashes.end(), lid) == clashes.end()) {
            clashes.push_back(lid);
        }
    }
    for (int i : ldata->teachers) {
        int lid = t_weeks[i][day][hour];
        if (lid && std::find(
                clashes.begin(), clashes.end(), lid) == clashes.end()) {
            clashes.push_back(lid);
        }
    }
    for (int i : ldata->rooms_needed) {
        int lid = r_weeks[i][day][hour];
        if (lid && std::find(
                clashes.begin(), clashes.end(), lid) == clashes.end()) {
            clashes.push_back(lid);
        }
    }
    if (!ldata->rooms_choice.empty()) {
        int lid;
        for (int i : ldata->rooms_choice) {
            lid = r_weeks[i][day][hour];
            if (lid && std::find(
                    clashes.begin(), clashes.end(), lid) != clashes.end()) {
                // The room is attached to a lesson which already clashes
                return clashes;
            }
        }
        // Take the last room's lesson as the clash, just so that
        // something gets listed (TODO: is there a better way?)
        clashes.push_back(lid);
    }
    return clashes;
}
