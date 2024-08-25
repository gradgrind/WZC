#ifndef FETDATA_H
#define FETDATA_H

#include <QJsonObject>
#include "readxml.h"

struct DBNode {
    int Id;
    QString DB_TABLE;
    QJsonObject DATA;
};

struct Day {
    int index;
    QString Name;
    QString Long_Name;
};

struct Hour {
    int index;
    QString Name;
    QString Long_Name;
    QString start;
    QString end;
};

struct Subject {
    int index;
    QString Name;
    QString Long_Name;
};

struct Teacher {
    int index;
    QString Name;
    QString Long_Name;
};

class FetData
{
public:
    FetData(XMLNode xmlin);

    QList<DBNode> nodeList;

    QMap<QString, int> days;
    QList<Day> day_list;
    QMap<QString, int> hours;
    QList<Hour> hour_list;
    QMap<QString, int> subjects;
    QList<Subject> subject_list;
    QMap<QString, int> teachers;
    QList<Teacher> teacher_list;

};

#endif // FETDATA_H
