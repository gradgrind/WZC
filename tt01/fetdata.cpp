#include "fetdata.h"

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
    qDebug() << smap;
    return smap;
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

    QMap<QString, Day> days;
    int i = 0;
    for (const auto &v : fet_top["Days_List"]) {
        auto n = v.value<XMLNode>();
        if (n.name == "Day") {
            auto m = readSimpleItems(n);
            auto name = m.value("Name");
            Day d{
                .index = i,
                .Name = name,
                .Long_Name = m.value("Long_Name")
            };
            days[name] = d;
            i++;
        }
    }

    QMap<QString, Hour> hours;
    i = 0;
    for (const auto &v : fet_top["Hours_List"]) {
        auto n = v.value<XMLNode>();
        if (n.name == "Hour") {
            auto m = readSimpleItems(n);
            auto name = m.value("Name");
            // Use special formatting in the long name to allow setting
            // of the start and end time
            auto lname = m.value("Long_Name").split('@');
            Hour h{
                .index = i,
                .Name = name,
                .Long_Name = lname[0]
            };
            if (lname.length() == 2) {
                auto t = lname[1].split('-');
                if (t.length() == 2) {
                    h.start = t[0];
                    h.end = t[1];
                }
            }
            hours[name] = h;
            i++;
        }
    }
}
