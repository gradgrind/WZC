#include "timetabledata.h"
#include <QJsonArray>

// p_class_divs is only used for testing purposes.
QString p_class_divs(class_divs cdivs)
{
    QStringList qsl0;
    for (const auto & dl : cdivs) {
        QStringList qsl1;
        for (const auto &item : dl) {
            auto qsl = QStringList(item.subgroups.values());
            auto s = QString("{%1 -> %2}").arg(
                QString::number(item.group), qsl.join(","));
            qsl1.append(s);
        }
        qsl0.append(qsl1.join(","));
    }
    return qsl0.join("|");
}


TimetableData::TimetableData(DBData * dbdata) : db_data{dbdata}
{
    // Make course lists for all classes and teachers.

    // Start with an analysis of the divisions and their groups/subgroups
    // for all classes. The results of this are used by the calls to
    // course_divisions.
    class_subgroup_divisions = class_divisions();

    // Collect the group tile information for each course,
    // add the course-ids to the lists for the classes and teachers.
    for (int course_id : dbdata->Tables["COURSES"]) {
        auto cdata = dbdata->Nodes[course_id];
        auto groups = cdata["GROUPS"].toArray();
        auto llist = course_divisions(groups);

        course_tileinfo[course_id] = llist;
        auto tlist = cdata.value("TEACHERS").toArray();
        for (const auto &t : tlist) {
            int tid = t.toInt();
            teacher_courses[tid].append(course_id);
        }
        for (auto iter = llist.cbegin(), end = llist.cend();
             iter != end; ++iter) {
            int cl = iter.key();
            class_courses[cl].append(course_id);
        }

        /*
        // Print the item
        QStringList ll;
        for (auto iter = llist.cbegin(), end = llist.cend();
                iter != end; ++iter) {
            int cl = iter.key();
            for (const auto &llf : iter.value()) {
                ll.append(QString("%1(%2:%3:%4:%5)")
                              .arg(cl)
                              .arg(llf.offset)
                              .arg(llf.fraction)
                              .arg(llf.total)
                              .arg(llf.groups.join(",")));
            }
        }
        QString subject = dbdata->get_tag(cdata.value("SUBJECT").toInt());
        QStringList teachers;
        for (const auto &t : tlist) {
            teachers.append(dbdata->get_tag(t.toInt()));
        }
        qDebug() << "COURSE TILES" << subject << teachers.join(",")
                 << groups << "->" << ll.join(",");
        */
    }
}

// Given a list of group indexes.
// Divide them into classes.
// Determine which division is used.
// Produce the fractional data, and potentially more than one tile per class.

// To assist, build subgroup sets for all classes,
// for each division a list of mappings: group -> subgroup-set.
// Each class should also have a set of all its subgroups, here as
// first division.

QHash<int, class_divs> TimetableData::class_divisions()
{
    // The division list contains lists of pairs: {gid, subgroup-set}.
    // The first division contains a single pair, for the whole class.
    QHash<int, class_divs> class_groups;
    // Loop through all classes
    for (int c : db_data->Tables["CLASSES"]) {
        class_divs divs;
        auto cdata = db_data->Nodes.value(c);
        auto divarray = cdata.value("DIVISIONS").toArray();
        for (const auto &d : divarray) {
            division_list divlist;
            auto groups = d.toObject().value("Groups").toArray();
            for (const auto &g : groups) {
                int gid = g.toInt();
                auto sglist = db_data->Nodes.value(gid)["SUBGROUPS"].toArray();
                QSet<QString> gset;
                for (const auto &sg : sglist) {
                    gset.insert(sg.toString());
                }
                divlist.append({{gid, gset}});
            }
            divs.append(divlist);
        }
        //qDebug() << "CLASS" << cdata["TAG"].toString() << p_class_divs(divs);
        class_groups[c] = divs;
    }
    return class_groups;
}

QMap<int, QList<TileFraction>> TimetableData::course_divisions(
    const QJsonArray groups)
{
    // For the results
    QMap<int, QList<TileFraction>> results;
    // Divide the groups into classes, collecting subgroups
    QMap<int, QSet<QString>> class_subgroups;
    for (const auto &v : groups) {
        int g = v.toInt();
        auto gnode = db_data->Nodes.value(g);
        int cl = gnode.value("CLASS").toInt();
        auto sglist = gnode.value("SUBGROUPS").toArray();
        for (const auto &sg : sglist) {
            class_subgroups[cl].insert(sg.toString());
        }
    }
    for (auto iter = class_subgroups.cbegin(), end = class_subgroups.cend();
            iter != end; ++iter) {
        int cl = iter.key();
        auto cgset = iter.value();
        auto sgdivs = class_subgroup_divisions.value(cl);
        for (const auto &sgdiv : sgdivs) {
            int offset = 0;
            int frac = 0;
            int i = 0;
            QStringList tags;
            for (const auto & sg : sgdiv) {
                i++;
                if (cgset.contains(sg.subgroups)) {
                    frac++;
                    tags.append(db_data->get_tag(sg.group));
                } else {
                    if (frac != 0) {
                        // emit tile data
                        results[cl].append({
                            offset, frac, int(sgdiv.length()), tags});
                        // restart
                        frac = 0;
                        tags.clear();
                    }
                    offset = i;
                }
            }
            if (frac != 0) {
                // emit any remaining tile data
                results[cl].append({
                    offset, frac, int(sgdiv.length()), tags});
            }
            if (results.contains(cl)) break;
        }
        if (!results.contains(cl)) {
            qFatal() << "Couldn't make tile for class" << cl << "from:" << groups;
        }
    }
    return results;
}
