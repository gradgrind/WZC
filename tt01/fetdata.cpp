#include "fetdata.h"
#include <QJsonArray>

// Note that QMultiMap returns the values of entries with multiple values
// in reverse order (as a QList)!
// It is possible to reverse such a list:
//     QList<QString> sl{"1", "2", "3", "4"};
//     std::reverse(sl.begin(), sl.end());
//     qDebug() << "REVERSED:" << sl;

QMultiMap<QString, QString> readSimpleItems(XMLNode node) {
    QMultiMap<QString, QString> smap;
    for (const auto &v : node.children) {
        auto n = v.value<XMLNode>();
        if (n.children.length() == 1) {
            auto c = n.children[0];
            if (c.canConvert<QString>()) {
                //qDebug() << n.name << "::" << c.toString();
                smap.insert(n.name, c.toString());
            }
        } else if (n.children.isEmpty()) {
            //qDebug() << n.name << ":: []";
            smap.insert(n.name, QString());
        }
    }
    //qDebug() << smap;
    return smap;
}

void readDays(QList<DBNode> &node_list, QList<QVariant> item_list)
{
    int i = 0;
    for (const auto &v : item_list) {
        auto n = v.value<XMLNode>();
        if (n.name == "Day") {
            auto m = readSimpleItems(n);
            auto name = m.value("Name");
            int id = node_list.length();
            node_list.append({
                .Id = id,
                .DB_TABLE = "DAYS",
                .DATA = {
                    {"ID", name},
                    {"NAME",  m.value("Long_Name")},
                    {"X", i}
                }
            });
            //qDebug() << id << node_list[id].DATA;
            i++;
        }
    }
}

void readHours(QList<DBNode> &node_list, QList<QVariant> item_list)
{
    int i = 0;
    for (const auto &v : item_list) {
        auto n = v.value<XMLNode>();
        if (n.name == "Hour") {
            auto m = readSimpleItems(n);
            auto name = m.value("Name");
            // Use special formatting in the long name to allow setting
            // of the start and end time
            auto lname = m.value("Long_Name").split('@');
            QString start, end;
            if (lname.length() == 2) {
                auto t = lname[1].split('-');
                if (t.length() == 2) {
                    start = t[0];
                    end = t[1];
                }
            }
            int id = node_list.length();
            node_list.append({
                .Id = id,
                .DB_TABLE = "HOURS",
                .DATA = {
                         {"ID", name},
                         {"NAME",  lname[0]},
                         {"X", i},
                         {"START_TIME", start},
                         {"END_TIME", end},
                         }
            });
            //qDebug() << id << node_list[id].DATA;
            i++;
        }
    }
}

void readSubjects(QList<DBNode> &node_list, QList<QVariant> item_list)
{
    int i = 0;
    for (const auto &v : item_list) {
        auto n = v.value<XMLNode>();
        if (n.name == "Subject") {
            auto m = readSimpleItems(n);
            auto name = m.value("Name");
            int id = node_list.length();
            node_list.append({
                .Id = id,
                .DB_TABLE = "SUBJECTS",
                .DATA = {
                    {"ID", name},
                    {"NAME",  m.value("Long_Name")},
                    {"X", i}
                }
            });
            //qDebug() << id << node_list[id].DATA;
            i++;
        }
    }
}

void readTeachers(QList<DBNode> &node_list, QList<QVariant> item_list)
{
    int i = 0;
    for (const auto &v : item_list) {
        auto n = v.value<XMLNode>();
        if (n.name == "Teacher") {
            auto m = readSimpleItems(n);
            auto name = m.value("Name");
            int id = node_list.length();
            node_list.append({
                .Id = id,
                .DB_TABLE = "TEACHERS",
                .DATA = {
                    {"ID", name},
                    {"NAME",  m.value("Long_Name")},
                    {"X", i}
                }
            });
            //qDebug() << id << node_list[id].DATA;
            i++;
        }
    }
}

void readRooms(QList<DBNode> &node_list, QList<QVariant> item_list)
{
    // Need mapping to indexes for the "ID" fields – for the real rooms
    // inside the virtual rooms.
    // Assume that the real rooms upon which the virtual rooms depend
    // appear before the virtual rooms which use them!
    QHash<QString, int> tag2ix;
    int i = 0;
    for (const auto &v : item_list) {
        auto n = v.value<XMLNode>();
        if (n.name == "Room") {
            auto m = readSimpleItems(n);
            auto name = m.value("Name");
            int id = node_list.length();
            QJsonObject data{
                        {"ID", name},
                        {"NAME",  m.value("Long_Name")},
                        {"X", i}
            };
            tag2ix[name] = id;
            if (m.value("Virtual") == "true") {
                QJsonArray vra;
                for (const auto &vc : n.children) {
                    auto nc = vc.value<XMLNode>();
                    if (nc.name == "Set_of_Real_Rooms") {
                        QJsonArray rra;
                        for (const auto &vr : nc.children) {
                            auto nr = vr.value<XMLNode>();
                            if (nr.name == "Real_Room") {
                                auto r = nr.children[0].toString();
                                // convert to index!
                                if (!tag2ix.contains(r)) {
                                    qFatal() << "Virtual Room contains room"
                                             << r << "(not yet defined)";
                                }
                                rra.append(tag2ix[r]);
                            }
                        }
                        vra.append(rra);
                    }
                }
                data["SUBROOMS"] = vra;
            }
            //TODO: There is probably a better way of handling this:
            auto c = m.value("Comments");
            if (!c.isEmpty()) {
                data["$REF"] = c;
            }
            node_list.append({
                .Id = id,
                .DB_TABLE = "ROOMS",
                .DATA = data
            });
            //qDebug() << id << node_list[id].DATA;
            i++;
        }
    }
}

void readClasses(QList<DBNode> &node_list, QList<QVariant> item_list)
{
    int i = 0;
    for (const auto &v : item_list) {
        auto n = v.value<XMLNode>();
        if (n.name == "Year") {
            auto m = readSimpleItems(n);
            auto name = m.value("Name");
            int id = node_list.length();

            QJsonArray categories;
            for (const auto &vc : n.children) {
                auto nc = vc.value<XMLNode>();
                if (nc.name == "Category") {
                    auto cdata = readSimpleItems(nc);
                    auto divs = cdata.values("Division");
                    std::reverse(divs.begin(), divs.end());
                    categories.append(QJsonArray::fromStringList(divs));
                } else if (nc.name == "Group") {
//TODO
                }

            }

            node_list.append({
                .Id = id,
                .DB_TABLE = "CLASSES",
                .DATA = {
                    {"ID", name},
                    {"NAME",  m.value("Long_Name")},
                    {"X", i},
                    {"DIVISIONS", categories},
                }
            });

            qDebug() << id << node_list[id].DATA;
            i++;
        }
    }
}

FetData::FetData(XMLNode xmlin)
{
    // Read the top level items
    qDebug() << xmlin.name << xmlin.attributes;
    QMap<QString, QList<QVariant>> fet_top;
    for (const auto &v : xmlin.children) {
        auto n = v.value<XMLNode>();
        fet_top[n.name] = n.children;
    }
    qDebug() << fet_top["Institution_Name"][0].toString();

    // Start the node-list with a dummy entry (index 0 is invalid).
    nodeList.append(DBNode{});

    readDays(nodeList, fet_top["Days_List"]);
    readHours(nodeList, fet_top["Hours_List"]);
    readSubjects(nodeList, fet_top["Subjects_List"]);
    readTeachers(nodeList, fet_top["Teachers_List"]);
    readRooms(nodeList, fet_top["Rooms_List"]);
    readClasses(nodeList, fet_top["Students_List"]);
    // <Activity_Tags_List> ???
}


    //TODO: Activities_List
    //TODO: Time_Constraints_List
    //TODO: Space_Constraints_List
