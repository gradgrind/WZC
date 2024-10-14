#include "database.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
//#include <QElapsedTimer>

DBData::DBData(QMap<int, QJsonObject> node_map) : Nodes {node_map}
{
    // Currently I assume that any editing of the basic data in "Nodes" will
    // also update the "Tables" structure appropriately.
    for (auto i = Nodes.cbegin(), end = Nodes.cend(); i != end; ++i) {
        Tables[i.value().value("TYPE").toString()].append(i.key());
    }
    reload();
}

void DBData::reload()
{
    // days: mapping, day's database-id -> 0-based day index
    const auto dlist = Tables.value("DAYS");
    days.clear();
    dix_id.resize(dlist.size());
    for (int d : dlist) {
        int dix = Nodes.value(d).value("X").toInt();
        days[d] = dix;
        dix_id[dix] = d;
    }
    // hours: mapping, hour's database-id -> 0-based hour index
    const auto hlist = Tables.value("HOURS");
    hours.clear();
    hix_id.resize(hlist.size());
    for (int h : hlist) {
        int hix = Nodes.value(h).value("X").toInt();
        hours[h] = hix;
        hix_id[hix] = h;
    }
    // Make lesson lists for the courses (values are lesson database-ids).
    course_lessons.clear();
    for (int lid : Tables["LESSONS"]) {
        auto ldata = Nodes[lid];
        course_lessons[ldata["COURSE"].toInt()].append(lid);
    }

}

QString DBData::get_tag(int id)
{
    return Nodes.value(id).value("TAG").toString();
}

void DBData::load(QString path)
{
    QSqlDatabase db = QSqlDatabase::database();
    if (db.isValid()) {
        db.close();
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE");
    }
    db.setDatabaseName(path);
    if (!db.open()) {
        qFatal() << "Opening database failed:" << path
                 << "\n Error:" << db.lastError();
    }
    db_path = path;
    QSqlQuery query("select Id, DATA from NODES");
    Nodes.clear();
    while (query.next()) {
        int i = query.value(0).toInt();
        QJsonParseError err;
        auto json = QJsonDocument::fromJson(
            query.value(1).toByteArray(), &err);
        if (json.isNull()) {
            qFatal() << "Bad JSON:" << err.errorString() << "in node" << i;
        } else if (!json.isObject()) {
            qFatal() << "Invalid JSON in node" << i;
        }
        Nodes[i] = json.object();
    }
}

void DBData::save(QString path)
{
    QSqlDatabase db = QSqlDatabase::database();
    if (db.isValid()) {
        db.close();
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE");
    }
    db.setDatabaseName(path);
    if (!db.open()) {
        qFatal() << "Opening database failed:" << path
                 << "\n Error:" << db.lastError();
    }
    db_path = path;
    save();
}

void DBData::save()
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid()) {
        qFatal() << "Saving data failed – no database open";
    }
    db.transaction(); // using a transaction makes a dramatic speed difference
    QSqlQuery query;
    query.exec("drop table if exists NODES");
    query.exec("create table NODES "
               "(Id integer primary key, "
               "DATA text)");
    query.prepare("insert into NODES (Id, DATA) "
                  "values (?, ?)");
    for (auto i = Nodes.cbegin(), end = Nodes.cend(); i != end; ++i) {
        query.bindValue(0, i.key());
        query.bindValue(1, QJsonDocument(i.value())
            .toJson(QJsonDocument::JsonFormat::Compact));
        query.exec();
    }
    db.commit();
}

int time2mins(QString time)
{
    auto hm = time.split(":");
    bool ok;
    int h = hm[0].toInt(&ok);
    if (!ok || h < 0 || h >= 24) return -1;
    int m = hm[1].toInt(&ok);
    if (!ok || m < 0 || m >= 60) return -1;
    return h * 60 + m;
}
