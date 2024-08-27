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

struct FetInfo{
    QHash<QString, int> days;
    QHash<QString, int> hours;
    QHash<QString, int> teachers;
    QHash<QString, int> subjects;
    QHash<QString, int> rooms;
    QHash<QString, int> groups;
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
