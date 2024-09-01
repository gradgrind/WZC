#include "basicconstraints.h"
#include <QJsonArray>

BasicConstraints::BasicConstraints(DBData *dbdata) : db_data{dbdata}
{
    // Each "resource" – (atomic) student group, teacher and room –
    // has a vector of slots for one week, organized as days * hours.
    // When the resource is "busy" in a time slot, the slot will contain
    // the index of the lesson-node.

    // Collect the atomic subgroups
    int sgi = 0;
    for (int gid : dbdata->Tables.value("GROUPS")) {
        auto node = dbdata->Nodes.value(gid).DATA;
        auto sglist = node.value("SUBGROUPS").toArray();
        bool allsg = node.value("ID").toString().isEmpty();
        for (auto sg : sglist) {
            auto sgstr = sg.toString();
            g2sg[gid].append(sgstr);
            if (allsg) {
                // A whole-class group
                sg2i[sgstr] = sgi;
                sgi++;
            }
        }
    }
    //qDebug() << "sg2i:" << sg2i;

    // Make a weekly array for each atomic subgroup
    ndays = dbdata->days.size();
    nhours = dbdata->hours.size();
    sg_weeks = std::vector<std::vector<std::vector<int>>>(
        sgi, std::vector<std::vector<int>> (
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
    int ri = 0;
    for (int rid : dbdata->Tables.value("ROOMS")) {
        auto node = dbdata->Nodes.value(rid).DATA;
        if (node.contains("SUBROOMS")) continue;
        // A real room
        r2i[rid] = ri;
        ri++;
    }
    r_weeks = std::vector<std::vector<std::vector<int>>>(
        ri, std::vector<std::vector<int>> (
            ndays , std::vector<int> (nhours)));

    slot_blockers();
    place_lessons();
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
        if (node.contains("SUBROOMS")) continue;
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
void BasicConstraints::place_lessons()
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
        //TODO: Need the possible-rooms-list (in "ROOMSPEC")
        auto rlist = node.value("ROOMSPEC").toArray();
        for (auto rlv : rlist) {
            std::vector<int> rilist;
            auto rl = rlv.toArray();
            for (auto r : rl) {
                rilist.push_back(r2i.value(r.toInt()));
            }
            ldc.roomspec.push_back(rilist);
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
            if (d0 == 0) continue;
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
