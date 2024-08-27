#include "lessontiles.h"
#include "fetdata.h"
#include <QJsonArray>
#include <QList>

// Given a list of group indexes.
// Divide them into classes.
// Determine which division is used.
// Produce the fractional data, and potentially more than one tile per class.

struct TileFraction {
    int offset;
    int fraction;
    int total;
};

void class_divisions(FetInfo &fet_info, QJsonArray groups)
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
            }
        }

    }
}
