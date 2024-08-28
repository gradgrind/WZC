#ifndef FETDATA_H
#define FETDATA_H

#include <QJsonObject>
#include "readxml.h"

struct DBNode {
    int Id;
    QString DB_TABLE;
    QJsonObject DATA;
};

QMultiMap<QString, QString> readSimpleItems(XMLNode node);

struct group_subgroups {
    int group;
    QSet<QString> subgroups;
};

typedef QList<group_subgroups> division_list;
typedef QList<division_list> class_divs;

struct FetInfo{
    QHash<QString, int> days;
    QHash<QString, int> hours;
    QHash<QString, int> teachers;
    QHash<QString, int> subjects;
    QHash<QString, int> rooms;
    QList<int> class_list;
    QHash<QString, int> groups;
    QHash<int, class_divs> class_subgroup_divisions;
    QHash<QString, int> activity_lesson;
    QList<DBNode> nodes;
};

class FetData
{
public:
    FetData(XMLNode xmlin);

    QList<DBNode> nodeList;
};

#endif // FETDATA_H
