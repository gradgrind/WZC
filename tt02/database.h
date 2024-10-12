#ifndef DATABASE_H
#define DATABASE_H

#include <QString>
#include <QList>
#include <QJsonObject>

// I had thought of a QJsonObject as database. This could have the
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
    QList<int> dix_id;      // dix_id[day index] -> day's db-index
    QHash<int, int> hours;  // db-index -> absolute index
    QList<int> hix_id;      // hix_id[hour index] -> hour's db-index
    QHash<int, QList<int>> course_lessons;

    DBData(QMap<int, QJsonObject> node_map);
    void reload();

    void load(QString path);
    void save(QString path);
    void save();
    QString get_tag(int id);
};

int time2mins(QString time);

#endif // DATABASE_H
