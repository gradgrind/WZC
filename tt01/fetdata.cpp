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

struct fetinfo{
    QHash<QString, int> teachers;
    QHash<QString, int> subjects;
    QHash<QString, int> rooms;
    QHash<QString, int> groups;
    QList<DBNode> nodes;
};

void readDays(fetinfo &fet_info, QList<QVariant> item_list)
{
    int i = 0;
    for (const auto &v : item_list) {
        auto n = v.value<XMLNode>();
        if (n.name == "Day") {
            auto m = readSimpleItems(n);
            auto name = m.value("Name");
            int id = fet_info.nodes.length();
            fet_info.nodes.append({
                .Id = id,
                .DB_TABLE = "DAYS",
                .DATA = {
                    {"ID", name},
                    {"NAME",  m.value("Long_Name")},
                    {"X", i}
                }
            });
            //qDebug() << id << fet_info.nodes[id].DATA;
            i++;
        }
    }
}

void readHours(fetinfo &fet_info, QList<QVariant> item_list)
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
            int id = fet_info.nodes.length();
            fet_info.nodes.append({
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
            //qDebug() << id << fet_info.nodes[id].DATA;
            i++;
        }
    }
}

void readSubjects(fetinfo &fet_info, QList<QVariant> item_list)
{
    int i = 0;
    for (const auto &v : item_list) {
        auto n = v.value<XMLNode>();
        if (n.name == "Subject") {
            auto m = readSimpleItems(n);
            auto name = m.value("Name");
            int id = fet_info.nodes.length();
            fet_info.nodes.append({
                .Id = id,
                .DB_TABLE = "SUBJECTS",
                .DATA = {
                    {"ID", name},
                    {"NAME",  m.value("Long_Name")},
                    {"X", i}
                }
            });
            fet_info.subjects[name] = id;
            //qDebug() << id << fet_info.nodes[id].DATA;
            i++;
        }
    }
}

void readTeachers(fetinfo &fet_info, QList<QVariant> item_list)
{
    int i = 0;
    for (const auto &v : item_list) {
        auto n = v.value<XMLNode>();
        if (n.name == "Teacher") {
            auto m = readSimpleItems(n);
            auto name = m.value("Name");
            int id = fet_info.nodes.length();
            fet_info.nodes.append({
                .Id = id,
                .DB_TABLE = "TEACHERS",
                .DATA = {
                    {"ID", name},
                    {"NAME",  m.value("Long_Name")},
                    {"X", i}
                }
            });
            fet_info.teachers[name] = id;
            //qDebug() << id << fet_info.nodes[id].DATA;
            i++;
        }
    }
}

void readRooms(fetinfo &fet_info, QList<QVariant> item_list)
{
    // Assume that the real rooms upon which the virtual rooms depend
    // appear before the virtual rooms which use them!
    int i = 0;
    for (const auto &v : item_list) {
        auto n = v.value<XMLNode>();
        if (n.name == "Room") {
            auto m = readSimpleItems(n);
            auto name = m.value("Name");
            int id = fet_info.nodes.length();
            QJsonObject data{
                        {"ID", name},
                        {"NAME",  m.value("Long_Name")},
                        {"X", i}
            };
            fet_info.rooms[name] = id;
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
                                if (!fet_info.rooms.contains(r)) {
                                    qFatal() << "Virtual Room contains room"
                                             << r << "(not yet defined)";
                                }
                                rra.append(fet_info.rooms[r]);
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
            fet_info.nodes.append({
                .Id = id,
                .DB_TABLE = "ROOMS",
                .DATA = data
            });
            //qDebug() << id << fet_info.nodes[id].DATA;
            i++;
        }
    }
}

// Note that this assumes a certain structure for the class data:
// The "Categories" are used to define the divisions.
// All groups must have appropriate subgroups defined.
// The subgroups are not used anywhere else in the fet-file, the
// students participating in all activities are defined only using
// classes (Years) and Groups.
void readClasses(fetinfo &fet_info, QList<QVariant> item_list)
{
    struct category{
        QString tag;
        QList<QString> groups;
    };

    int i = 0;
    for (const auto &v : item_list) {
        auto n = v.value<XMLNode>();
        if (n.name == "Year") {
            auto m = readSimpleItems(n);
            auto name = m.value("Name");
            auto sep = m.value("Separator");
            // Collect the groups
            QList<category> categories;
            // Collect group indexes (key without class part)
            QMap<QString, int> group2index;
            // Collect subgroups
            QSet<QString> allsubgroups;
            //QJsonArray groups;
            for (const auto &vc : n.children) {
                auto nc = vc.value<XMLNode>();
                if (nc.name == "Category") {
                    auto cdata = readSimpleItems(nc);
                    auto divs = cdata.values("Division");
                    std::reverse(divs.begin(), divs.end());
                    auto tag = QStringList(divs).join(",");
                    categories.append({
                        .tag = tag,
                        .groups = divs,
                    });
                } else if (nc.name == "Group") {
                    QJsonArray subgroups;
                    auto cdata = readSimpleItems(nc);
                    for (const auto &vsg : nc.children) {
                        auto nsg = vsg.value<XMLNode>();
                        if (nsg.name == "Subgroup") {
                            auto sgdata = readSimpleItems(nsg);
                            auto sgname = sgdata.value("Name");
                            subgroups.append(sgname);
                            allsubgroups.insert(sgname);
                        }
                    }
                    auto g0 = cdata.value("Name");
                    auto gid = g0.split(sep);
                    int id = fet_info.nodes.length();
                    fet_info.nodes.append({
                        .Id = id,
                        .DB_TABLE = "GROUPS",
                        .DATA = {
                            {"ID", gid[1]},
                            {"CLASS", name},
                            {"SUBGROUPS", subgroups},
                            {"STUDENTS", QJsonArray()},
                        }
                    });
                    fet_info.groups[g0] = id;
                    group2index[gid[1]] = id;
                }
            }
            // Add a group entry for the full class
            int id = fet_info.nodes.length();
            auto sglist = allsubgroups.values();
            std::sort(sglist.begin(), sglist.end());
            fet_info.nodes.append({
                .Id = id,
                .DB_TABLE = "GROUPS",
                .DATA = {
                    {"ID", ""},
                    {"CLASS", name},
                    {"SUBGROUPS", QJsonArray::fromStringList(sglist)},
                    {"STUDENTS", QJsonArray()},
                }
            });
            fet_info.groups[name] = id;
            // Convert the division members to indexes
            QJsonArray divisions;
            for (const auto &d : categories) {
                QJsonArray glist;
                for (const auto &g : d.groups) {
                    int gid = group2index.value(g);
                    if (gid) {
                        glist.append(gid);
                    } else {
                        qFatal() << "Class" << name << "::"
                                 << "Group" << g << "not defined!";
                    }
                }
                divisions.append(QJsonObject{
                    {"Tag", d.tag},
                    {"Groups", glist},
                });
            }
            id = fet_info.nodes.length();
            fet_info.nodes.append({
                .Id = id,
                .DB_TABLE = "CLASSES",
                .DATA = {
                    {"ID", name},
                    {"NAME",  m.value("Long_Name")},
                    {"X", i},
                    {"DIVISIONS", divisions},
                }
            });
            i++;

            /*
            for (auto iter = group2index.cbegin(), end = group2index.cend();
                    iter != end; ++iter) {
                qDebug() << "  GROUP" << iter.key() << "::" << iter.value()
                         << fet_info.nodes[iter.value()].DATA;
            }
            qDebug() << "  GROUP" << "*" << "::" << id-1
                     << fet_info.nodes[id-1].DATA;
            qDebug() << id << fet_info.nodes[id].DATA;
            */
        }
    }
}

void readActivities(fetinfo &fet_info, QList<QVariant> item_list)
{
    // The Activity_Group_Id refers to a course. If it is 0
    // that is a single-activity course. Otherwise there are
    // multiple activities, so the course must be cached.
    QHash<QString, int> coursemap;
    for (const auto &v : item_list) {
        auto n = v.value<XMLNode>();
        if (n.name == "Activity") {
            auto m = readSimpleItems(n);
            auto cid = m.value("Activity_Group_Id");
            int id = coursemap.value(cid);
            if (id == 0) {
                int id = fet_info.nodes.length();
                qDebug() << "  ACTIVITY" << m.value("Id") << cid << id;
                QJsonArray tlist;
                for (const auto &t : m.values("Teacher")) {
                    int tid = fet_info.teachers.value(t);
                    if (tid) {
                        tlist.append(tid);
                    } else {
                        qFatal() << "Activity" << m.value("Id")
                                 << "has unknown teacher:" << t;
                    }
                }
                auto s = m.value("Subject");
                int sid = fet_info.subjects.value(s);
                if (!sid) {
                    qFatal() << "Activity" << m.value("Id")
                    << "has unknown subject:" << s;
                }
                QJsonArray glist;
                for (const auto &g : m.values("Students")) {
                    int gid = fet_info.groups.value(g);
                    if (gid) {
                        glist.append(gid);
                    } else {
                        qFatal() << "Activity" << m.value("Id")
                        << "has unknown group:" << g;
                    }
                }
                fet_info.nodes.append({
                    .Id = id,
                    .DB_TABLE = "COURSES",
                    .DATA = {
                        {"TEACHERS", tlist},
                        {"SUBJECT", sid},
                        {"STUDENTS", glist},
                    },
                });
                if (cid != "0") {
                    coursemap[cid] = id;
                }
            } else {
                qDebug() << "  ACTIVITY" << m.value("Id") << id;
            }

            //TODO: Make LESSONS

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

    fetinfo fetdata;
    // Start the node-list with a dummy entry (index 0 is invalid).
    fetdata.nodes.append(DBNode{});

    readDays(fetdata, fet_top["Days_List"]);
    readHours(fetdata, fet_top["Hours_List"]);
    readSubjects(fetdata, fet_top["Subjects_List"]);
    readTeachers(fetdata, fet_top["Teachers_List"]);
    readRooms(fetdata, fet_top["Rooms_List"]);
    readClasses(fetdata, fet_top["Students_List"]);

    qDebug() << "§§§1" << fetdata.nodes.length();
    readActivities(fetdata, fet_top["Activities_List"]);
    qDebug() << "§§§2" << fetdata.nodes.length();
    // <Activity_Tags_List> ???
}


    //TODO: Activities_List
    //TODO: Time_Constraints_List
    //TODO: Space_Constraints_List
