#include "readspaceconstraints.h"

void readSpaceConstraints(FetInfo &fet_info, QList<QVariant> item_list)
{
    for (const auto &v : item_list) {
        auto n = v.value<XMLNode>();
        if (n.name == "XXX") {
            auto m = readSimpleItems(n);
            auto name = m.value("Name");
            int id = fet_info.nodes.length();
            fet_info.nodes.append({
                .Id = id,
                .DB_TABLE = "TEACHERS",
                .DATA = {
                         {"ID", name},
                         {"NAME",  m.value("Long_Name")},
                         }
            });
            fet_info.teachers[name] = id;
            //qDebug() << id << fet_info.nodes[id].DATA;
        }
    }

}
