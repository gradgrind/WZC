#include "lessontiles.h"
#include <QJsonArray>

// Given a list of group indexes.
// Divide them into classes.
// Determine which division is used.
// Produce the fractional data, and potentially more than one tile per class.

// Maybe it would be a good idea to build subgroup sets for all classes.
// For each division a list of mappings: group -> subgroup-set.
// Each class should also have a set of all its subgroups, perhaps as
// first division?

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

void class_divisions(FetInfo &fet_info)
{
    // The division list contains lists of pairs: {gid, subgroup-set}.
    // The first division contains a single pair, for the whole class.
    QHash<int, class_divs> class_groups;
    // Loop through all classes
    for (int c : fet_info.class_list) {
        class_divs divs;
        auto cdata = fet_info.nodes.value(c).DATA;
        auto cname = cdata["ID"].toString();
        //qDebug() << "CLASS" << cname;
        // Whole class
        int gid = fet_info.groups.value(cname);
        auto sglist = fet_info.nodes.value(gid).DATA["SUBGROUPS"].toArray();
        QSet<QString> gset;
        for (const auto &sg : sglist) {
            gset.insert(sg.toString());
        }
        division_list divlist;
        divlist.append({{gid, gset}});
        divs.append(divlist);
        // Now the actual divisions
        auto divarray = cdata.value("DIVISIONS").toArray();
        for (const auto &d : divarray) {
            division_list divlist;
            auto groups = d.toObject().value("Groups").toArray();
            for (const auto &g : groups) {
                int gid = g.toInt();
                auto sglist = fet_info.nodes.value(gid)
                                  .DATA["SUBGROUPS"].toArray();
                QSet<QString> gset;
                for (const auto &sg : sglist) {
                    gset.insert(sg.toString());
                }
                divlist.append({{gid, gset}});
            }
            divs.append(divlist);
        }
        //qDebug() << "CLASS" << cname << p_class_divs(divs);
        class_groups[c] = divs;
    }
    fet_info.class_subgroup_divisions = class_groups;
}

void course_divisions(FetInfo &fet_info, QJsonArray groups)
{
    // For the results
    QMap<int, QList<TileFraction>> results;
    // Divide the groups into classes, collecting subgroups
    QMap<int, QSet<QString>> class_subgroups;
    for (const auto &v : groups) {
        int g = v.toInt();
        auto gnode = fet_info.nodes.value(g);
        int cl = gnode.DATA.value("CLASS").toInt();
        auto sglist = gnode.DATA.value("SUBGROUPS").toArray();
        for (const auto &sg : sglist) {
            class_subgroups[cl].insert(sg.toString());
        }
    }
    for (auto i = class_subgroups.cbegin(), end = class_subgroups.cend();
            i != end; ++i) {
        int cl = i.key();

        auto sgdivs = fet_info.class_subgroup_divisions.value(cl);
//TODO ...
        for (const auto &sgdiv : sgdivs) {
            for (const auto & sg : sgdiv) {

            }
        }

        auto cnode = fet_info.nodes.value(cl);
        // Get the list of all subgroups for the class – this must go via
        // the class name to the whole-class group
        QString cname = cnode.DATA.value("ID").toString();
        auto cgnode = fet_info.nodes.value(fet_info.groups.value(cname));
        auto allsg = cgnode.DATA.value("SUBGROUPS").toArray();
        auto sgset = i.value();
        if (sgset.size() == allsg.size()) {
            // Whole class
            results[cl].append({0, 1, 1});
            continue;
        }
        // Find the correct division
        auto divs = cnode.DATA.value("DIVISIONS").toArray();
        for (const auto &d : divs) {
            auto groups = d.toObject().value("Groups").toArray();
            for (const auto &g : groups) {
                //TODO
                int gid = g.toInt();
                auto gnode = fet_info.nodes.value(gid);
                auto sg = gnode.DATA.value("SUBGROUPS").toArray();


            }
        }

    }
}
