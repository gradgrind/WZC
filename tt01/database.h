#ifndef DATABASE_H
#define DATABASE_H

#include <QString>
#include <QList>
#include <QJsonObject>

struct DBNode {
    int Id;
    QString DB_TABLE;
    QJsonObject DATA;
};

struct TileFraction {
    int offset;
    int fraction;
    int total;
    QStringList groups;
};

struct group_subgroups {
    int group;
    QSet<QString> subgroups;
};

typedef QList<group_subgroups> division_list;
typedef QList<division_list> class_divs;

class DBData
{
public:
    QString db_path;
    QList<DBNode> Nodes;
    QHash<QString, QList<int>> Tables;
    QHash<int, int> days;   // db-index -> absolute index
    QHash<int, int> hours;  // db-index -> absolute index
    QHash<int, class_divs> class_subgroup_divisions;
    QHash<int, QMap<int, QList<TileFraction>>> course_tileinfo;
    QHash<int, QList<int>> teacher_courses;
    QHash<int, QList<int>> class_courses;
    QHash<int, QList<int>> course_lessons;
    //QHash<int, QList<int>> room_courses;
    // It may be better to seek the courses for a room dynamically,
    // as these might be changed interactively.


    DBData(QList<DBNode> nodes);

    void save(QString path);
    void save();
    QString get_tag(int index);
};

QString get_tag(QList<DBNode> nodes, int index);

void save_data(QString path, QList<DBNode> nodes);

#endif // DATABASE_H
