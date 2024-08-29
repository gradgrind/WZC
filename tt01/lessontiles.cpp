#include "lessontiles.h"
#include <QJsonArray>

// Given a list of group indexes.
// Divide them into classes.
// Determine which division is used.
// Produce the fractional data, and potentially more than one tile per class.

// To assist, build subgroup sets for all classes,
// for each division a list of mappings: group -> subgroup-set.
// Each class should also have a set of all its subgroups, here as
// first division.

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

void class_divisions(DBData &db_data)
{
    // The division list contains lists of pairs: {gid, subgroup-set}.
    // The first division contains a single pair, for the whole class.
    QHash<int, class_divs> class_groups;
    // Loop through all classes
    for (int c : db_data.Tables["CLASSES"]) {
        class_divs divs;
        auto cdata = db_data.Nodes.value(c).DATA;
        auto divarray = cdata.value("DIVISIONS").toArray();
        for (const auto &d : divarray) {
            division_list divlist;
            auto groups = d.toObject().value("Groups").toArray();
            for (const auto &g : groups) {
                int gid = g.toInt();
                auto sglist = db_data.Nodes.value(gid)
                                  .DATA["SUBGROUPS"].toArray();
                QSet<QString> gset;
                for (const auto &sg : sglist) {
                    gset.insert(sg.toString());
                }
                divlist.append({{gid, gset}});
            }
            divs.append(divlist);
        }
        //qDebug() << "CLASS" << cdata["ID"].toString() << p_class_divs(divs);
        class_groups[c] = divs;
    }
    db_data.class_subgroup_divisions = class_groups;
}

QMap<int, QList<TileFraction>> course_divisions(
    DBData &db_data, QJsonArray groups)
{
    // For the results
    QMap<int, QList<TileFraction>> results;
    // Divide the groups into classes, collecting subgroups
    QMap<int, QSet<QString>> class_subgroups;
    for (const auto &v : groups) {
        int g = v.toInt();
        auto gnode = db_data.Nodes.value(g);
        int cl = gnode.DATA.value("CLASS").toInt();
        auto sglist = gnode.DATA.value("SUBGROUPS").toArray();
        for (const auto &sg : sglist) {
            class_subgroups[cl].insert(sg.toString());
        }
    }
    for (auto iter = class_subgroups.cbegin(), end = class_subgroups.cend();
            iter != end; ++iter) {
        int cl = iter.key();
        auto cgset = iter.value();
        auto sgdivs = db_data.class_subgroup_divisions.value(cl);
        for (const auto &sgdiv : sgdivs) {
            int offset = 0;
            int frac = 0;
            int i = 0;
            QStringList tags;
            for (const auto & sg : sgdiv) {
                i++;
                if (cgset.contains(sg.subgroups)) {
                    frac++;
                    tags.append(db_data.get_tag(sg.group));
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
