#ifndef DATABASE_H
#define DATABASE_H

#include <QString>
#include <QList>
#include <QJsonObject>



//TODO-- I had thought of a QJsonObject as database. This could have the
// "correct" internal indexes of the elements. But editing this structure
// would be practically impossible because of the potentially mangled references!
// That was why I chose the sqlite form in the first place.
// However, I guess the references can also make managing edits difficult with
// the sqlite form too. I would need to seek references which are to be changed,
// and know what to do with them. In that respect the general id-numbers of the
// nodes is quite a good idea. The ordering is not so important, and it may
// make sense to use a mapping instead of a list. For tracing references it
// may be helpful to collect them in a further table, keyed by the destination?
// Such an extra table wouldn't be needed in the stored data, it should be
// possible to construct it on loading.

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
    QMap<int, QJsonObject> Nodes; // use QMap rather then QHash, for sorted keys
    QHash<QString, QList<int>> Tables;
    QHash<int, int> days;   // db-index -> absolute index
    QHash<int, int> hours;  // db-index -> absolute index
    QHash<int, class_divs> class_subgroup_divisions;
    // The tiles are divided only for the class view. The map below
    // supplies a list of tiles for each involved class.
    QHash<int, QMap<int, QList<TileFraction>>> course_tileinfo;
    QHash<int, QList<int>> teacher_courses;
    QHash<int, QList<int>> class_courses;
    QHash<int, QList<int>> course_lessons;
    //QHash<int, QList<int>> room_courses;
    // It may be better to seek the courses for a room dynamically,
    // as these might be changed interactively.

    DBData(QMap<int, QJsonObject> node_map);

    void load(QString path);
    void save(QString path);
    void save();
    QString get_tag(int id);
};

int time2mins(QString time);

#endif // DATABASE_H
