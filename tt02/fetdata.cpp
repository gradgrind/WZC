#include "fetdata.h"
#include <QJsonArray>
#include "readtimeconstraints.h"
#include "readspaceconstraints.h"

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

void readDays(FetInfo &fet_info, QList<QVariant> item_list)
{
    int i = 0;
    for (const auto &v : item_list) {
        auto n = v.value<XMLNode>();
        if (n.name == "Day") {
            auto m = readSimpleItems(n);
            auto name = m.value("Name");
            int id = fet_info.next_index();
            fet_info.nodes[id] = QJsonObject{
                {"Id", id},
                {"TYPE", "DAYS"},
                {"TAG", name},
                {"NAME",  m.value("Long_Name")},
                {"X", i},
            };
            fet_info.days[name] = id;
            //qDebug() << id << fet_info.nodes[id];
            i++;
        }
    }
}

void readHours(FetInfo &fet_info, QList<QVariant> item_list)
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
            int id = fet_info.next_index();
            fet_info.nodes[id] = QJsonObject{
                {"Id", id},
                {"TYPE", "HOURS"},
                {"TAG", name},
                {"NAME", lname[0]},
                {"X", i},
                {"START_TIME", start},
                {"END_TIME", end},
            };
            fet_info.hours[name] = id;
            //qDebug() << id << fet_info.nodes[id].DATA;
            i++;
        }
    }
}

void readSubjects(FetInfo &fet_info, QList<QVariant> item_list)
{
    int i = 0;
    for (const auto &v : item_list) {
        auto n = v.value<XMLNode>();
        if (n.name == "Subject") {
            auto m = readSimpleItems(n);
            auto name = m.value("Name");
            int id = fet_info.next_index();
            fet_info.nodes[id] = QJsonObject{
                {"Id", id},
                {"TYPE", "SUBJECTS"},
                {"TAG", name},
                {"NAME", m.value("Long_Name")},
                {"X", i},
            };
            fet_info.subjects[name] = id;
            //qDebug() << id << fet_info.nodes[id].DATA;
            i++;
        }
    }
}

void readTeachers(FetInfo &fet_info, QList<QVariant> item_list)
{
    int i = 0;
    for (const auto &v : item_list) {
        auto n = v.value<XMLNode>();
        if (n.name == "Teacher") {
            auto m = readSimpleItems(n);
            auto name = m.value("Name");
            int id = fet_info.next_index();
            fet_info.nodes[id] = QJsonObject{
                {"Id", id},
                {"TYPE", "TEACHERS"},
                {"TAG", name},
                {"NAME", m.value("Long_Name")},
                {"X", i},
            };
            fet_info.teachers[name] = id;
            //qDebug() << id << fet_info.nodes[id].DATA;
            i++;
        }
    }
}

void readRooms(FetInfo &fet_info, QList<QVariant> item_list)
{
    // Assume that the real rooms upon which the virtual rooms depend
    // appear before the virtual rooms which use them!
    // Only a simplified subset of FETs possibilities is supported here.
    // Specifically, virtual rooms may contain any number of single-member
    // sets (i.e. compulsory rooms), but only one set with more than one room
    // (after elimination of any rooms specified as compulsory in other sets).
    // Additional multi-room sets will be ignored.
    int i = 0;
    for (const auto &v : item_list) {
        auto n = v.value<XMLNode>();
        if (n.name == "Room") {
            auto m = readSimpleItems(n);
            auto name = m.value("Name");
            int id = fet_info.next_index();
            QJsonObject data{
                {"Id", id},
                {"TYPE", "ROOMS"},
                {"TAG", name},
                {"NAME", m.value("Long_Name")},
                {"X", i},
            };
            fet_info.rooms[name] = id;
            if (m.value("Virtual") == "true") {
                QList<int> compulsory;
                QList<QList<int>> choices;
                // "FIXED_ROOMS" is the list of necessary rooms,
                // "ROOM_CHOICE" is the list from which one room must be taken.
                for (const auto &vc : n.children) {
                    auto nc = vc.value<XMLNode>();
                    if (nc.name == "Set_of_Real_Rooms") {
                        QList<int> choices1;
                        for (const auto &vr : nc.children) {
                            auto nr = vr.value<XMLNode>();
                            if (nr.name == "Real_Room") {
                                auto r = nr.children[0].toString();
                                // convert to index!
                                if (!fet_info.rooms.contains(r)) {
                                    qFatal() << "Virtual Room contains room"
                                             << r << "(not yet defined)";
                                }
                                choices1.append(fet_info.rooms[r]);
                            }
                        }
                        if (choices1.size() > 1) {
                            choices.append(choices1);
                        } else {
                            compulsory.append(choices1[0]);
                        }
                    }
                }
                // Remove any rooms from the choices lists which are in
                // the compulsory list.
                QJsonArray choice_array;
                while (!choices.empty()) {
                    bool extended = false;
                    QList<QList<int>> choices2;
                    for (const auto & clist : choices) {
                        QList<int> clist2;
                        for (int r : clist) {
                            if (!compulsory.contains(r)) clist2.append(r);
                        }
                        if (clist2.length() > 1) choices2.append(clist2);
                        else if (!clist2.empty()) {
                            compulsory.append(clist[0]);
                            extended = true;
                        }
                    }
                    if (!extended) {
                        // The lists have been maximally shortened. Take the
                        // first one as the choices list.
                        if (!choices2.empty()) {
                            for (int r : choices2[0]) {
                                choice_array.append(r);
                            }
                            data["ROOM_CHOICE"] = choice_array;
                            if (choices2.length() > 1) {
                                qWarning() << "Virtual room" << name
                                           << "has more than one choice list"
                                           << "(excess ignored)";
                            }
                        }
                        break;
                    }
                    // Otherwise repeat with the modified lists
                    choices = choices2;
                }
                // Even if it is empty the FIXED_ROOMS field should be present
                // to act as a flag for the virtual nature of this room.
                QJsonArray compulsory_array;
                for (int r : compulsory) {
                    compulsory_array.append(r);
                }
                data["FIXED_ROOMS"] = compulsory_array;
            }
            //TODO: There is probably a better way of handling this:
            auto c = m.value("Comments");
            if (!c.isEmpty()) {
                data["$REF"] = c;
            }
            fet_info.nodes[id] = data;
            //qDebug() << id << fet_info.nodes[id].DATA;
            i++;
        }
    }
}

// Note that this assumes a certain structure for the class data:
// The "Categories" are used to define the divisions.
// All groups must have appropriate subgroups defined, even if there is
// only a single subgroup. Also undivided classes must have a subgroup.
// The subgroups are not used anywhere else in the fet-file, the
// students participating in all activities are defined only using
// classes (Years) and Groups.
void readClasses(FetInfo &fet_info, QList<QVariant> item_list)
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
            int clid = fet_info.next_index();
            fet_info.nodes[clid] = QJsonObject{
                {"Id", clid},
                {"TYPE", "CLASSES"},
                {"TAG", name},
                {"NAME", m.value("Long_Name")},
                {"X", i},
            };
            fet_info.class_list.append(clid);
            i++;
            // Collect the groups
            QList<category> categories;
            // Collect group indexes (key without class part)
            QMap<QString, int> group2index;
            // Collect subgroups
            QSet<QString> allsubgroups;
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
                    int id = fet_info.next_index();
                    fet_info.nodes[id] = QJsonObject{
                        {"Id", id},
                        {"TYPE", "GROUPS"},
                        {"TAG", gid[1]},
                        {"CLASS", clid},
                        {"SUBGROUPS", subgroups},
                        {"STUDENTS", QJsonArray()},
                    };
                    fet_info.groups[g0] = id;
                    group2index[gid[1]] = id;
                }
            }
            // Add a group entry for the full class.
            // At present this is not referrred to by the class node!
            auto sglist = allsubgroups.values();
            if (sglist.length() > 1) {
                std::sort(sglist.begin(), sglist.end());
            } else if (sglist.isEmpty()) {
                sglist.append(name + ":0");
            }
            int id = fet_info.next_index();
            fet_info.nodes[id] = QJsonObject{
                {"Id", id},
                {"TYPE", "GROUPS"},
                {"TAG", ""},
                {"CLASS", clid},
                {"SUBGROUPS", QJsonArray::fromStringList(sglist)},
                {"STUDENTS", QJsonArray()},
            };
            fet_info.groups[name] = id;
            // Rebuild the divisions, starting with the whole-class "division"
            QJsonArray divisions{QJsonObject{
                {"DivTag", "*"},
                {"Groups", QJsonArray{id}},
            }};
            // Convert the remaining division members to indexes
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
                    {"DivTag", d.tag},
                    {"Groups", glist},
                });
            }
            // Set the divisions on the class
            fet_info.nodes[clid]["DIVISIONS"] = divisions;

            /*
            qDebug() << clid << "CLASS" << fet_info.nodes[clid];
            while (clid < id) {
                clid++;
                qDebug() << clid << "  GROUP" << fet_info.nodes[clid];
            }
            */
        }
    }
}

void readActivities(FetInfo &fet_info, QList<QVariant> item_list)
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
                        qCritical() << "Activity" << m.value("Id")
                                    << "has unknown group:" << g;
                    }
                }
                id = fet_info.next_index();
                fet_info.nodes[id] = QJsonObject{
                    {"Id", id},
                    {"TYPE", "COURSES"},
                    {"TEACHERS", tlist},
                    {"SUBJECT", sid},
                    {"GROUPS", glist},
                };
                fet_info.course_list.append(id);
                if (cid != "0") {
                    coursemap[cid] = id;
                }
            }
            // Make LESSONS node
            int lid = fet_info.next_index();

            auto data = QJsonObject{
                {"Id", lid},
                {"TYPE", "LESSONS"},
                {"COURSE", id},
                {"LENGTH", m.value("Duration").toInt()},
                {"ACTIVITY_TAGS", QJsonArray::fromStringList(
                                      m.values("Activity_Tag"))},
            };
            //TODO: There is probably a better way of handling this:
            auto c = m.value("Comments");
            if (!c.isEmpty()) data["$REF"] = c;
            fet_info.nodes[lid] = data;
            fet_info.activity_lesson[m.value("Id")] = lid;
        }
    }
}

FetInfo fetData(XMLNode xmlin)
{
    // Read the top level items
    qDebug() << xmlin.name << xmlin.attributes;
    if (xmlin.name != "fet") {
        qFatal() << "Invalid fet file";
    }
    QMap<QString, QList<QVariant>> fet_top;
    for (const auto &v : xmlin.children) {
        auto n = v.value<XMLNode>();
        fet_top[n.name] = n.children;
    }
    auto iname = fet_top["Institution_Name"];
    if (iname.size() != 0)
        qDebug() << fet_top["Institution_Name"][0].toString();

    FetInfo fetdata;
    readDays(fetdata, fet_top["Days_List"]);
    readHours(fetdata, fet_top["Hours_List"]);
    readSubjects(fetdata, fet_top["Subjects_List"]);
    readTeachers(fetdata, fet_top["Teachers_List"]);
    readRooms(fetdata, fet_top["Rooms_List"]);
    readClasses(fetdata, fet_top["Students_List"]);
    readActivities(fetdata, fet_top["Activities_List"]);
    readTimeConstraints(fetdata, fet_top["Time_Constraints_List"]);
    readSpaceConstraints(fetdata, fet_top["Space_Constraints_List"]);
    // <Activity_Tags_List> ???

    return fetdata;
}
